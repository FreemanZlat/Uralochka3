#include "evaluate.h"

#include <iostream>

Evaluate::Evaluate()
{
    EvalParams &params = EvalParams::instance();

    for (int i = 0; i < 64; ++i)
    {
        for (int j = 0; j < 64; ++j)
            this->distance[i][j] = std::max(abs(Bitboards::_col[i]-Bitboards::_col[j]),
                                            abs(Bitboards::_row[i]-Bitboards::_row[j]));
    }
}

void Evaluate::init(Board *board)
{
    EvalParams &params = EvalParams::instance();
    this->_board = board;
}

// Оценочная функция
// Возвращает оценку позиции в сантипешках. Если >0, то перевес стороны, чей ход. Иначе - перевес врага.
int Evaluate::eval(int ply, int alpha, int beta)
{
#ifdef USE_NN
    int res = this->eval_neural(ply);
#else
    int res = this->eval_classic();
#endif
    if (res > 2000)
        res = 2000 + (res - 2000) / 5;
    else if (res < -2000)
        res = -2000 + (res + 2000) / 5;
    // Возвращаем окончательную оценку, в зависимости от того, чей ход
    return this->_board->is_white(ply) ? res : -res;
}

int Evaluate::eval_classic()
{
    Bitboards &bitboards = Bitboards::instance();
    EvalParams &params = EvalParams::instance();

    int eval_mg = 0, eval_eg = 0;

    this->_all = this->_board->_all_pieces[0] | this->_board->_all_pieces[1];

    this->_pawn_attacks[0] = ((this->_board->_bitboards[0][Board::PAWN] & ~Bitboards::_cols[7]) << 9) |
            ((this->_board->_bitboards[0][Board::PAWN] & ~Bitboards::_cols[0]) << 7);
    this->_pawn_attacks[1] = ((this->_board->_bitboards[1][Board::PAWN] & ~Bitboards::_cols[7]) >> 7) |
            ((this->_board->_bitboards[1][Board::PAWN] & ~Bitboards::_cols[0]) >> 9);

    u64 pawns_blocked_white = Bitboards::pawns_moves(this->_all, ~this->_board->_bitboards[0][Board::PAWN], 1);
    u64 pawns_blocked_black = Bitboards::pawns_moves(this->_all, ~this->_board->_bitboards[1][Board::PAWN], 0);

    this->_game_phase = 0;
    this->_all_minus_bishops[0] = this->_all ^ (this->_board->_bitboards[0][Board::BISHOP] | this->_board->_bitboards[0][Board::QUEEN]);
    this->_all_minus_bishops[1] = this->_all ^ (this->_board->_bitboards[1][Board::BISHOP] | this->_board->_bitboards[1][Board::QUEEN]);
    this->_all_minus_rooks[0] = this->_all ^ (this->_board->_bitboards[0][Board::ROOK] | this->_board->_bitboards[0][Board::QUEEN]);
    this->_all_minus_rooks[1] = this->_all ^ (this->_board->_bitboards[1][Board::ROOK] | this->_board->_bitboards[1][Board::QUEEN]);

    this->_mobility[0] = ~(this->_board->_bitboards[0][Board::KING] | this->_pawn_attacks[1] | pawns_blocked_white);
    this->_mobility[1] = ~(this->_board->_bitboards[1][Board::KING] | this->_pawn_attacks[0] | pawns_blocked_black);

    this->_attacks[0] = this->_pawn_attacks[0] | bitboards._moves_king[this->_board->_wking];
    this->_attacks[1] = this->_pawn_attacks[1] | bitboards._moves_king[this->_board->_bking];

    this->_king_area[0] = bitboards._moves_king[this->_board->_wking] | (1ull << this->_board->_wking);
    this->_king_area[1] = bitboards._moves_king[this->_board->_bking] | (1ull << this->_board->_bking);

    this->_king_attackers[0] = 0;
    this->_king_attackers[1] = 0;

    this->_king_attackers_weight_mg[0] = 0;
    this->_king_attackers_weight_mg[1] = 0;
    this->_king_attackers_weight_eg[0] = 0;
    this->_king_attackers_weight_eg[1] = 0;

    this->eval_queens(params, 0, eval_mg, eval_eg);
    this->eval_queens(params, 1, eval_mg, eval_eg);

    this->eval_rooks(params, 0, eval_mg, eval_eg);
    this->eval_rooks(params, 1, eval_mg, eval_eg);

    this->eval_bishops(params, 0, eval_mg, eval_eg);
    this->eval_bishops(params, 1, eval_mg, eval_eg);

    this->eval_knights(params, 0, eval_mg, eval_eg);
    this->eval_knights(params, 1, eval_mg, eval_eg);

    this->eval_pawns(params, 0, eval_mg, eval_eg);
    this->eval_pawns(params, 1, eval_mg, eval_eg);

    this->eval_kings(params, 0, eval_mg, eval_eg);
    this->eval_kings(params, 1, eval_mg, eval_eg);

    eval_eg = this->endgame_scale(params, eval_eg);

    if (this->_game_phase > params._values[EvalParams::GAME_PHASE][EvalParams::MG][0])
        this->_game_phase = params._values[EvalParams::GAME_PHASE][EvalParams::MG][0];

    return (eval_mg * this->_game_phase + eval_eg * (params._values[EvalParams::GAME_PHASE][EvalParams::MG][0] - this->_game_phase)) /
            params._values[EvalParams::GAME_PHASE][EvalParams::MG][0];
}

int Evaluate::eval_neural(int ply)
{
    // Нейронка, нафиг!
/*
    int neural = this->_board->_neural.predict_i(
                    this->_board->color(ply),
                    this->_board->_bitboards[0][Board::KING],
                    this->_board->_bitboards[1][Board::KING],
                    this->_board->_bitboards[0][Board::PAWN],
                    this->_board->_bitboards[1][Board::PAWN],
                    this->_board->_bitboards[0][Board::KNIGHT],
                    this->_board->_bitboards[1][Board::KNIGHT],
                    this->_board->_bitboards[0][Board::BISHOP],
                    this->_board->_bitboards[1][Board::BISHOP],
                    this->_board->_bitboards[0][Board::ROOK],
                    this->_board->_bitboards[1][Board::ROOK],
                    this->_board->_bitboards[0][Board::QUEEN],
                    this->_board->_bitboards[1][Board::QUEEN]
                );
*/
    int neural = this->_board->_neural.accum_predict(this->_board->_stack_pointer, this->_board->color(ply),
                                                     Neural::stage(Bitboards::bits_count(this->_board->_all_pieces[0] | this->_board->_all_pieces[1]),
                                                                   (this->_board->_bitboards[0][Board::QUEEN] | this->_board->_bitboards[1][Board::QUEEN]) != 0));

    int dtz = this->_board->get_dtz();
    if (dtz < 60)
        neural = neural * dtz / 60;

    return neural;
}

int Evaluate::eval_flipped(int ply)
{
    int res = this->eval(ply, -20000, 20000);

    Board *board = this->_board;

    Board board_flipped;

    board_flipped._nodes[0]._flags = board->_nodes[ply]._flags ^ Board::FLAG_WHITE_MOVE;

    board_flipped._all_pieces[0] = 0;
    board_flipped._all_pieces[1] = 0;
    for (int i = 0; i < 7; ++i)
    {
        board_flipped._bitboards[0][i] = 0;
        board_flipped._bitboards[1][i] = 0;
    }

    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
        {
            int sq = row*8 + col;
            int sq_flipped = (7-row)*8 + col;

            int piece = board->_board[sq];
            int piece_flipped = 0;

            int piece_type = piece & Board::PIECES_MASK;
            if (piece_type != 0)
            {

                if (piece & Board::WHITE_MASK)
                {
                    piece_flipped = piece_type | Board::BLACK_MASK;

                    if (piece_type == Board::KING)
                        board_flipped._bking = sq_flipped;

                    Bitboards::bit_set(board_flipped._bitboards[1][piece_type], sq_flipped);
                    Bitboards::bit_set(board_flipped._all_pieces[1], sq_flipped);
                }
                else
                {
                    piece_flipped = piece_type | Board::WHITE_MASK;

                    if (piece_type == Board::KING)
                        board_flipped._wking = sq_flipped;

                    Bitboards::bit_set(board_flipped._bitboards[0][piece_type], sq_flipped);
                    Bitboards::bit_set(board_flipped._all_pieces[0], sq_flipped);
                }
            }

            board_flipped._board[sq_flipped] = piece_flipped;
        }


    this->_board = &board_flipped;
    int res_flipped = this->eval(0, -20000, 20000);
    this->_board = board;

    if (res != res_flipped)
    {
        board->print();
        board_flipped.print();
        std::cout << "eval: " << res << std::endl;
        std::cout << "eval_flipped: " << res_flipped << std::endl << std::endl;
    }

    return res;
}

void Evaluate::eval_kings(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    int pos = color ? this->_board->_bking : this->_board->_wking;
    int mg = color ? params.pst_black_mg[Board::KING][pos] : params.pst_white_mg[Board::KING][pos];
    int eg = color ? params.pst_black_eg[Board::KING][pos] : params.pst_white_eg[Board::KING][pos];

    int col = Bitboards::_col[pos];
    int row = Bitboards::_row[pos];

    u64 pawns = this->_board->_bitboards[color][Board::PAWN];
//    u64 pawns_enemy = this->_board->_bitboards[1-color][Board::PAWN];

    for (int i = -1; i <= 1; ++i)
    {
        if (col+i < 0 || col+i > 7)
            continue;

        u64 pawns_forward = pawns & bitboards._cols_forward[color][pos+i];
        int dist = 6;
        if (pawns_forward != 0)
            dist = (color ? row - Bitboards::_row[Bitboards::msb(pawns_forward)] : Bitboards::_row[Bitboards::lsb(pawns_forward)] - row) - 1;
        int idx = (col+i) * 7 + dist;
        mg += params._values[EvalParams::KING_SHIELD][EvalParams::MG][idx];
    }

    u64 defenders = this->_board->_bitboards[color][Board::PAWN] |
            this->_board->_bitboards[color][Board::KNIGHT] |
            this->_board->_bitboards[color][Board::BISHOP];
    int defenders_count = Bitboards::bits_count(defenders & this->_king_area[color]);
    mg += params._values[EvalParams::KING_DEFENDERS][EvalParams::MG][defenders_count];
    eg += params._values[EvalParams::KING_DEFENDERS][EvalParams::EG][defenders_count];

    if (this->_king_attackers[color] > std::max(0, 1 - Bitboards::bits_count(this->_board->_bitboards[1-color][Board::QUEEN])))
    {
        int attackers = std::min(4, this->_king_attackers[color]);
        mg += params._values[EvalParams::ATTACKER_VALUE][EvalParams::MG][attackers] * this->_king_attackers_weight_mg[color] /
                params._values[EvalParams::ATTACKER_SCALE][EvalParams::MG][0];
        eg += params._values[EvalParams::ATTACKER_VALUE][EvalParams::EG][attackers] * this->_king_attackers_weight_eg[color] /
                params._values[EvalParams::ATTACKER_SCALE][EvalParams::EG][0];
    }

    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

void Evaluate::eval_queens(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    int mg = 0, eg = 0;
    this->_count_q[color] = 0;
    u64 pieces = this->_board->_bitboards[color][Board::QUEEN];
    while (pieces != 0)
    {
        this->_count_q[color]++;
        this->_game_phase += params._values[EvalParams::GAME_PHASE][EvalParams::MG][Board::QUEEN];
        int pos = Bitboards::poplsb(pieces);

        auto &magic1 = bitboards._magic_bishops[pos];
        auto &magic2 = bitboards._magic_rooks[pos];
        u64 attacks = magic1._attacks[magic1.index(this->_all_minus_bishops[color])] | magic2._attacks[magic2.index(this->_all_minus_rooks[color])];
        this->_attacks[color] |= attacks;
        int count = Bitboards::bits_count(attacks & this->_mobility[color]);
        mg += params._values[EvalParams::QUEEN_MOBILITY][EvalParams::MG][count];
        eg += params._values[EvalParams::QUEEN_MOBILITY][EvalParams::EG][count];

        attacks &= this->_king_area[1-color];
        if (attacks != 0)
        {
            this->_king_attackers[1-color]++;
            this->_king_attackers_weight_mg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::MG][Board::QUEEN];
            this->_king_attackers_weight_eg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::EG][Board::QUEEN];
        }

        mg += color ? params.pst_black_mg[Board::QUEEN][pos] : params.pst_white_mg[Board::QUEEN][pos];
        eg += color ? params.pst_black_eg[Board::QUEEN][pos] : params.pst_white_eg[Board::QUEEN][pos];
    }
/*
    if (this->_count_q[color] > 1)
    {
        int count = this->_count_q[color] - 1;
        mg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::QUEEN];
        eg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::QUEEN];
        this->_count_q[color] = 1;
    }
*/
    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

void Evaluate::eval_rooks(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    int mg = 0, eg = 0;
    int row7 = color ? 1 : 6;
    int row8 = color ? 0 : 7;
    int king_enemy_row = Bitboards::_row[color ? this->_board->_wking : this->_board->_bking];
    bool king_enemy_row78 = king_enemy_row == row7 || king_enemy_row == row8;
    this->_count_r[color] = 0;
    u64 pieces = this->_board->_bitboards[color][Board::ROOK];
    while (pieces != 0)
    {
        this->_count_r[color]++;
        this->_game_phase += params._values[EvalParams::GAME_PHASE][EvalParams::MG][Board::ROOK];
        int pos = Bitboards::poplsb(pieces);

        int col = Bitboards::_col[pos];
        int row = Bitboards::_row[pos];

        if (king_enemy_row78 && row == row7)
        {
            mg += params._values[EvalParams::ROOK_ON7][EvalParams::MG][0];
            eg += params._values[EvalParams::ROOK_ON7][EvalParams::EG][0];
        }

        if ((Bitboards::_cols[col] & this->_board->_bitboards[color][Board::PAWN]) == 0)
        {
            int flag = (Bitboards::_cols[col] & this->_board->_bitboards[1-color][Board::PAWN]) ? 1 : 0;
            mg += params._values[EvalParams::ROOK_ON_OPEN][EvalParams::MG][flag];
            eg += params._values[EvalParams::ROOK_ON_OPEN][EvalParams::EG][flag];
        }

        auto &magic = bitboards._magic_rooks[pos];
        u64 attacks = magic._attacks[magic.index(this->_all_minus_rooks[color])];
        this->_attacks[color] |= attacks;
        int count = Bitboards::bits_count(attacks & this->_mobility[color]);
        mg += params._values[EvalParams::ROOK_MOBILITY][EvalParams::MG][count];
        eg += params._values[EvalParams::ROOK_MOBILITY][EvalParams::EG][count];

        attacks &= this->_king_area[1-color];
        if (attacks != 0)
        {
            this->_king_attackers[1-color]++;
            this->_king_attackers_weight_mg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::MG][Board::ROOK];
            this->_king_attackers_weight_eg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::EG][Board::ROOK];
        }

        mg += color ? params.pst_black_mg[Board::ROOK][pos] : params.pst_white_mg[Board::ROOK][pos];
        eg += color ? params.pst_black_eg[Board::ROOK][pos] : params.pst_white_eg[Board::ROOK][pos];
    }
/*
    if (this->_count_r[color] > 2)
    {
        int count = this->_count_r[color] - 2;
        mg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::ROOK];
        eg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::ROOK];
        this->_count_r[color] = 2;
    }
*/
    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

void Evaluate::eval_bishops(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    int mg = 0, eg = 0;
    this->_count_bl[color] = 0;
    this->_count_bd[color] = 0;
    int count = 0;
    u64 pawns = this->_board->_bitboards[color][Board::PAWN] | this->_board->_bitboards[1-color][Board::PAWN];
    u64 pieces = this->_board->_bitboards[color][Board::BISHOP];
    while (pieces != 0)
    {
        count++;
        this->_game_phase += params._values[EvalParams::GAME_PHASE][EvalParams::MG][Board::BISHOP];
        int pos = Bitboards::poplsb(pieces);

        if (bitboards._dark[pos])
            this->_count_bd[color]++;
        else
            this->_count_bl[color]++;

        if (Bitboards::bit_test(Bitboards::pawns_moves(pawns, 0ull, 1-color), pos))
        {
            mg += params._values[EvalParams::BISHOP_BEHIND_PAWN][EvalParams::MG][0];
            eg += params._values[EvalParams::BISHOP_BEHIND_PAWN][EvalParams::EG][0];
        }

        auto &magic = bitboards._magic_bishops[pos];
        u64 attacks = magic._attacks[magic.index(this->_all_minus_bishops[color])];
        this->_attacks[color] |= attacks;
        int count = Bitboards::bits_count(attacks & this->_mobility[color]);
        mg += params._values[EvalParams::BISHOP_MOBILITY][EvalParams::MG][count];
        eg += params._values[EvalParams::BISHOP_MOBILITY][EvalParams::EG][count];

        attacks &= this->_king_area[1-color];
        if (attacks != 0)
        {
            this->_king_attackers[1-color]++;
            this->_king_attackers_weight_mg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::MG][Board::BISHOP];
            this->_king_attackers_weight_eg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::EG][Board::BISHOP];
        }

        mg += color ? params.pst_black_mg[Board::BISHOP][pos] : params.pst_white_mg[Board::BISHOP][pos];
        eg += color ? params.pst_black_eg[Board::BISHOP][pos] : params.pst_white_eg[Board::BISHOP][pos];
    }
/*
    if (this->_count_bl[color] > 1)
    {
        int count = this->_count_bl[color] - 1;
        mg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::BISHOP];
        eg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::BISHOP];
        this->_count_bl[color] = 1;
    }

    if (this->_count_bd[color] > 1)
    {
        int count = this->_count_bd[color] - 1;
        mg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::BISHOP];
        eg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::BISHOP];
        this->_count_bd[color] = 1;
    }
*/
    if (this->_count_bd[color] + this->_count_bl[color] > 1)
    {
        mg += params._values[EvalParams::TWO_BISHOPS][EvalParams::MG][0];
        eg += params._values[EvalParams::TWO_BISHOPS][EvalParams::EG][0];
    }

    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

void Evaluate::eval_knights(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    int mg = 0, eg = 0;
    this->_count_n[color] = 0;
    u64 pawns = this->_board->_bitboards[color][Board::PAWN] | this->_board->_bitboards[1-color][Board::PAWN];
    u64 pieces = this->_board->_bitboards[color][Board::KNIGHT];
    while (pieces != 0)
    {
        this->_count_n[color]++;
        this->_game_phase += params._values[EvalParams::GAME_PHASE][EvalParams::MG][Board::KNIGHT];
        int pos = Bitboards::poplsb(pieces);

        if (Bitboards::bit_test(Bitboards::pawns_moves(pawns, 0ull, 1-color), pos))
        {
            mg += params._values[EvalParams::KNIGHT_BEHIND_PAWN][EvalParams::MG][0];
            eg += params._values[EvalParams::KNIGHT_BEHIND_PAWN][EvalParams::EG][0];
        }

        u64 attacks = bitboards._moves_knight[pos];
        this->_attacks[color] |= attacks;
        int count = Bitboards::bits_count(attacks & this->_mobility[color]);
        mg += params._values[EvalParams::KNIGHT_MOBILITY][EvalParams::MG][count];
        eg += params._values[EvalParams::KNIGHT_MOBILITY][EvalParams::EG][count];

        attacks &= this->_king_area[1-color];
        if (attacks != 0)
        {
            this->_king_attackers[1-color]++;
            this->_king_attackers_weight_mg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::MG][Board::KNIGHT];
            this->_king_attackers_weight_eg[1-color] += params._values[EvalParams::ATTACKER_WEIGHT][EvalParams::EG][Board::KNIGHT];
        }

        mg += color ? params.pst_black_mg[Board::KNIGHT][pos] : params.pst_white_mg[Board::KNIGHT][pos];
        eg += color ? params.pst_black_eg[Board::KNIGHT][pos] : params.pst_white_eg[Board::KNIGHT][pos];
    }
/*
    if (this->_count_n[color] > 2)
    {
        int count = this->_count_n[color] - 2;
        mg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::KNIGHT];
        eg += count * params._values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::KNIGHT];
        this->_count_n[color] = 2;
    }
*/
    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

void Evaluate::eval_pawns(EvalParams &params, int color, int &eval_mg, int &eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

    u64 attacks = this->_king_area[1-color] & this->_pawn_attacks[color];
    if (attacks != 0)
        this->_king_attackers[1-color]++;

    int pawn_move = color ? -8 : 8;
    int mg = 0, eg = 0;
    this->_count_p[color] = 0;
    int king = color ? this->_board->_bking : this->_board->_wking;
    int king_enemy = color ? this->_board->_wking : this->_board->_bking;
    u64 pawns_enemy = this->_board->_bitboards[1-color][Board::PAWN];
    u64 pawns = this->_board->_bitboards[color][Board::PAWN];

    u64 pieces = pawns;
    while (pieces != 0)
    {
        this->_count_p[color]++;
        this->_game_phase += params._values[EvalParams::GAME_PHASE][EvalParams::MG][Board::PAWN];
        int pos = Bitboards::poplsb(pieces);

        mg += color ? params.pst_black_mg[Board::PAWN][pos] : params.pst_white_mg[Board::PAWN][pos];
        eg += color ? params.pst_black_eg[Board::PAWN][pos] : params.pst_white_eg[Board::PAWN][pos];

        int col = Bitboards::_col[pos];
        int row = Bitboards::_row[pos];
        if (color)
            row = 7-row;

        u64 col_pawns = pawns & Bitboards::_cols[col];
        u64 neighbors = bitboards._isolated[col] & pawns;
        u64 forward_attacks = bitboards._attacks_pawn[color][pos + pawn_move] & pawns_enemy;
        u64 backup = bitboards._passed[1-color][pos] & pawns;

        if (Bitboards::several(col_pawns))
        {
            mg += params._values[EvalParams::PAWN_DOUBLE][EvalParams::MG][col];
            eg += params._values[EvalParams::PAWN_DOUBLE][EvalParams::EG][col];
        }

        if (neighbors == 0)
        {
            mg += params._values[EvalParams::PAWN_ISOLATED][EvalParams::MG][col];
            eg += params._values[EvalParams::PAWN_ISOLATED][EvalParams::EG][col];
        }

        if (neighbors != 0 && forward_attacks != 0 && backup == 0)
        {
            if ((bitboards._cols_forward[color][pos] & pawns_enemy) != 0)
            {
                mg += params._values[EvalParams::PAWN_BACKWARD][EvalParams::MG][row];
                eg += params._values[EvalParams::PAWN_BACKWARD][EvalParams::EG][row];
            }
            else
            {
                mg += params._values[EvalParams::PAWN_BACKWARD_BLOCKED][EvalParams::MG][row];
                eg += params._values[EvalParams::PAWN_BACKWARD_BLOCKED][EvalParams::EG][row];
            }
        }

        if ((bitboards._passed[color][pos] & pawns_enemy) == 0)
        {
            u64 forward = (1ull << (pos + pawn_move));
            float flag = 1.0f;
            if ((forward & this->_board->_all_pieces[1-color]) != 0)
                flag = 0.6f;
            else if ((forward & this->_board->_all_pieces[color]) != 0)
                flag = 0.8f;
            else if ((forward & this->_attacks[1-color]) != 0)
                flag = 0.85f;
            else if ((bitboards._cols_forward[color][pos] & this->_board->_all_pieces[1-color]) != 0)
                flag = 0.7f;

            mg += flag * params._values[EvalParams::PAWN_PASSED][EvalParams::MG][row];
            eg += flag * params._values[EvalParams::PAWN_PASSED][EvalParams::EG][row];

            if ((pawns & bitboards._cols_forward[color][pos]) != 0)
                continue;

            int dist = this->distance[pos][king];
            mg += dist * params._values[EvalParams::PAWN_PASSED_KING][EvalParams::MG][row];
            eg += dist * params._values[EvalParams::PAWN_PASSED_KING][EvalParams::EG][row];

            dist = this->distance[pos][king_enemy];
            mg += dist * params._values[EvalParams::PAWN_PASSED_KING_ENEMY][EvalParams::MG][row];
            eg += dist * params._values[EvalParams::PAWN_PASSED_KING_ENEMY][EvalParams::EG][row];
        }
    }

    eval_mg += color ? -mg : mg;
    eval_eg += color ? -eg : eg;
}

int Evaluate::endgame_scale(EvalParams &params, int eval_eg)
{
    Bitboards &bitboards = Bitboards::instance();

//    int sign = eval_eg == 0 ? 0 : eval_eg < 0 ? -1 : 1;

//    u64 pawns = this->_board->_bitboards[0][Board::PAWN] | this->_board->_bitboards[1][Board::PAWN];

    int knights = this->_count_n[0] + this->_count_n[1];
    int bishops = this->_count_bd[0] + this->_count_bl[0] + this->_count_bd[1] + this->_count_bl[1];
    int rooks = this->_count_r[0] + this->_count_r[1];
    int queens = this->_count_q[0] + this->_count_q[1];

//    int complexity = params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::COMPLEXITY_DEFAULT];
//    complexity += params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::COMPLEXITY_PAWNS] * (this->_count_p[0] + this->_count_p[1]);
//    if ((pawns & bitboards._board_left) && (pawns & bitboards._board_right))
//        complexity += params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::COMPLEXITY_PAWNS_FLANKS];
//    if (knights + bishops + rooks + queens == 0)
//        complexity += params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::COMPLEXITY_PAWNS_EG];

//    eval_eg += sign * std::max(complexity, -abs(eval_eg));

    int scale_normal = params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_NORMAL];

    if (this->_count_bd[0] + this->_count_bl[0] == 1 && this->_count_bd[1] + this->_count_bl[1] == 1 && this->_count_bd[0] == this->_count_bl[1])
    {
        if (rooks + queens == 0 && this->_count_n[0] == 1 && this->_count_n[1] == 1)
            return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_OCB_KNIGHT] / scale_normal;

        if (knights + queens == 0 && this->_count_r[0] == 1 && this->_count_r[1] == 1)
            return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_OCB_ROOK] / scale_normal;

        if (knights + rooks + queens == 0)
        {
            if (abs(this->_count_p[0] - this->_count_p[1]) > 2)
                return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_OCB_BISHOPS_PAWNS] / scale_normal;
            else
                return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_OCB_BISHOPS] / scale_normal;
        }
    }

    int strong = eval_eg < 0 ? 1 : 0;

    int minors = knights + bishops;
    int pieces = minors + rooks;

    int minors_w = this->_count_n[0] + this->_count_bd[0] + this->_count_bl[0];
    int pieces_w = minors_w + this->_count_r[0];

    int minors_b = this->_count_n[1] + this->_count_bd[1] + this->_count_bl[1];
    int pieces_b = minors_b + this->_count_r[1];

    int pieces_weak = strong ? pieces_w : pieces_b;
    int minors_strong = strong ? minors_b : minors_w;

    // Lone Queens are weak against multiple pieces
    if (queens == 1 && pieces > 1 && pieces == pieces_weak)
        return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_QUEEN] / scale_normal;

    // Lone Minor vs King + Pawns should never be won
    if (minors_strong == 1 && this->_count_r[strong] + this->_count_q[strong] +this->_count_p[strong]  == 0)
        return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_DRAW] / scale_normal;

    // Scale up lone pieces with massive pawn advantages
    if (queens == 0 && pieces_w <= 1 && pieces_b <= 1 && this->_count_p[strong] - this->_count_p[1-strong] > 2)
        return eval_eg * params._values[EvalParams::ENDGAME_SCALE][EvalParams::EG][EvalParams::SCALE_LARGE_PAWN_ADV] / scale_normal;

    // Scale down as the number of pawns of the strong side reduces
    return eval_eg * std::min(static_cast<int>(scale_normal), 96 + this->_count_p[strong] * 8) / scale_normal;
}
