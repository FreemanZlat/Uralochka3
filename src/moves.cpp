#include "moves.h"

#include "board.h"

#include <algorithm>
#include <cstdio>

static const std::vector<int> &PICES_VALUES = { 0, 20000, 1300, 650, 400, 400, 100 };

static const std::vector<char> &SYMBOLS = { '*', 'k', 'q', 'r', 'b', 'n', 'p' };


Move::Move(u16 move, int value):
    _move(move),
    _value(value)
{
}

std::string Move::get_string()
{
    return Move::get_string(this->_move);
}

std::string Move::get_string(u16 move)
{
    // Строка хода
    std::string move_str = "";
    int from = move & 63;
    int to = (move >> 6) & 63;
    int morph = (move >> 12) & 7;
    move_str += static_cast<char>(from & 7) + 'a';
    move_str += static_cast<char>(from >> 3) + '1';
    move_str += static_cast<char>(to & 7) + 'a';
    move_str += static_cast<char>(to >> 3) + '1';
    if (morph != 0)
        move_str += SYMBOLS[morph & Board::PIECES_MASK];
    return move_str;
}

History::History()
{
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 64; ++j)
            for (auto &item: this->_history[i][j])
                item = 0;
        for (int j = 0; j < 7; ++j)
            for (int k = 0; k < 64; ++k)
            {
                for (int l = 0; l < 7; ++l)
                    for (auto &item: this->_followers[i][j][k][l])
                        item = 0;
                this->_counter_moves[i][j][k] = 0;
            }
    }
}

Moves::Moves()
{
    this->_hist_moves.reserve(128);
    this->_moves_kills.reserve(24);
    this->_moves_quiet.reserve(96);
    this->_killer1 = 0;
    this->_killer2 = 0;
    this->_counter = 0;
    this->clear();
}

void Moves::init(Board *board, History *history)
{
    this->_board = board;
    this->_history = history;
}

void Moves::clear_killers()
{
    this->_killer1 = 0;
    this->_killer2 = 0;
}

void Moves::init_generator(bool kills, int ply, u16 move_counter, int piece_counter, u16 move_follower, int piece_follower)
{
    this->clear();
    this->_ply = ply;
    this->_kills = kills;
    this->_stage = kills ? STAGE_GEN_KILLS : STAGE_HASH;
    this->_move_counter = move_counter;
    this->_piece_counter = piece_counter;
    this->_move_follower = move_follower;
    this->_piece_follower = piece_follower;
    if (move_counter != 0)
        this->_counter = this->_history->_counter_moves[this->_board->color(ply)][piece_counter][(move_counter >> 6) & 63];
    else
        this->_counter = 0;
}

void Moves::update_hash(u16 move)
{
    this->_hash = move;
}

void Moves::update_history(int depth, int color)
{
    int count = this->_hist_moves.size() - 1;
    if (count == 0 && depth <= 3)
        return;

    int bonus = depth*depth;
    if (bonus > 400)
        bonus = 400;

    int good_from = this->_hist_moves[count] & 63;
    int good_to = (this->_hist_moves[count] >> 6) & 63;
    int good_piece = this->_hist_moves[count] >> 16;

    int counter_to = (this->_move_counter >> 6) & 63;
    int follower_to = (this->_move_follower >> 6) & 63;

    history_bonus(this->_history->_history[color][good_from][good_to], bonus);
    if (this->_move_counter != 0)
    {
        history_bonus(this->_history->_followers[0][this->_piece_counter][counter_to][good_piece][good_to], bonus);
        this->_history->_counter_moves[color][this->_piece_counter][counter_to] = this->_hist_moves[count] & 65535;
    }
    if (this->_move_follower != 0)
        history_bonus(this->_history->_followers[1][this->_piece_follower][follower_to][good_piece][good_to], bonus);
    for (int i = 0; i < count; ++i)
    {
        int bad_from = this->_hist_moves[i] & 63;
        int bad_to = (this->_hist_moves[i] >> 6) & 63;
        int bad_piece = this->_hist_moves[i] >> 16;
        history_bonus(this->_history->_history[color][bad_from][bad_to], -bonus);
        if (this->_move_counter != 0)
            history_bonus(this->_history->_followers[0][this->_piece_counter][counter_to][bad_piece][bad_to], -bonus);
        if (this->_move_follower != 0)
            history_bonus(this->_history->_followers[1][this->_piece_follower][follower_to][bad_piece][bad_to], -bonus);
    }
}

void Moves::update_killers(u16 move)
{
    if (this->_killer1 == move)
        return;
    this->_killer2 = this->_killer1;
    this->_killer1 = move;
}

int Moves::get_history(int color, u16 move, int piece, int &counter_hist, int &follower_hist)
{
    int move_to = (move >> 6) & 63;
    counter_hist = 0;
    follower_hist = 0;
    if (this->_move_counter != 0)
    {
        int to = (this->_move_counter >> 6) & 63;
        counter_hist = this->_history->_followers[0][this->_piece_counter][to][piece][move_to];
    }
    if (this->_move_follower != 0)
    {
        int to = (this->_move_follower >> 6) & 63;
        follower_hist = this->_history->_followers[1][this->_piece_follower][to][piece][move_to];
    }
    return this->_history->_history[color][move & 63][move_to] + counter_hist + follower_hist;
}

u16 Moves::get_next(bool skip_quiets)
{
    switch (this->_stage)
    {
    case STAGE_HASH:
        if (!this->_generated)
        {
            this->_generated = true;
            this->gen_kills();
            this->gen_quiet();
        }
        this->_stage = STAGE_GEN_KILLS;
        if (this->_hash_move != 0)
        {
            return this->_hash_move;
//            if (this->is_legal(this->_hash_move))
//                return this->_hash_move;
//            else
//                this->_hash_move = 0;
        }

    case STAGE_GEN_KILLS:
        if (!this->_generated)
        {
            this->_generated = true;
            this->gen_kills();
        }
//        this->gen_kills();
        this->_stage = STAGE_GOOD_KILLS;

    case STAGE_GOOD_KILLS:
        if (this->_moves_kills.size() > 0)
        {
            int max = this->_moves_kills[0]._value;
            int max_i = 0;
            for (int i = 1; i < this->_moves_kills.size(); ++i)
                if (this->_moves_kills[i]._value > max)
                {
                    max = this->_moves_kills[i]._value;
                    max_i = i;
                }
            u16 res_move = this->_moves_kills[max_i]._move;
            if (max >= 0)
            {
                this->_moves_kills[max_i] = this->_moves_kills.back();
                this->_moves_kills.pop_back();
                return res_move;
            }
        }
        if (this->_kills)
            return 0;
        this->_stage = STAGE_KILLER1;

    case STAGE_KILLER1:
        this->_stage = STAGE_KILLER2;
        if (this->_killer1_move != 0/* && this->is_legal(this->_killer1_move)*/)
            return this->_killer1_move;

    case STAGE_KILLER2:
        this->_stage = STAGE_COUNTER;
        if (this->_killer2_move != 0/* && this->is_legal(this->_killer2_move)*/)
            return this->_killer2_move;

    case STAGE_COUNTER:
        this->_stage = STAGE_GEN_QUIET;
        if (this->_counter_move != 0/* && this->is_legal(this->_counter_move)*/)
            return this->_counter_move;

    case STAGE_GEN_QUIET:
//        this->gen_quiet();
        this->_stage = STAGE_QUIET;

    case STAGE_QUIET:
        if (!skip_quiets && this->_moves_quiet.size() > 0)
        {
            int max = this->_moves_quiet[0]._value;
            int max_i = 0;
            for (int i = 1; i < this->_moves_quiet.size(); ++i)
                if (this->_moves_quiet[i]._value > max)
                {
                    max = this->_moves_quiet[i]._value;
                    max_i = i;
                }
            u16 res_move = this->_moves_quiet[max_i]._move;
            this->_moves_quiet[max_i] = this->_moves_quiet.back();
            this->_moves_quiet.pop_back();
            return res_move;
        }
        this->_stage = STAGE_BAD_KILLS;

    case STAGE_BAD_KILLS:
        if (this->_moves_kills.size() > 0)
        {
            int max = this->_moves_kills[0]._value;
            int max_i = 0;
            for (int i = 1; i < this->_moves_kills.size(); ++i)
                if (this->_moves_kills[i]._value > max)
                {
                    max = this->_moves_kills[i]._value;
                    max_i = i;
                }
            u16 res_move = this->_moves_kills[max_i]._move;
            this->_moves_kills[max_i] = this->_moves_kills.back();
            this->_moves_kills.pop_back();
            return res_move;
        }
        this->_stage = STAGE_END;

    case STAGE_END:
        return 0;
    }
    return 0;
}

bool Moves::is_killer(u16 move)
{
    return move == this->_killer1 || move == this->_killer2 || move == this->_counter;
}

int Moves::SEE(u16 move)
{
    int color = this->_board->color(this->_ply);
    int from = move & 63;
    int to = (move >> 6) & 63;
    int pawn_morph = (move >> 12) & 7;
    int piece = this->_board->_board[from] & Board::PIECES_MASK;
    int killed = move & Move::KILLED;

    if (killed != 0)
    {
        killed = this->_board->_board[to] & Board::PIECES_MASK;
        if (killed == 0)
            killed = Board::PAWN;
    }

    int eval = PICES_VALUES[killed];
    if (pawn_morph > 0)
    {
        eval += PICES_VALUES[pawn_morph] - PICES_VALUES[piece];
        piece = pawn_morph;
    }

    u64 all = this->_board->_all_pieces[0] | this->_board->_all_pieces[1];
    // Убрать взятие на проходе из all!!!
    all ^= 1ull << from;
    return -this->see(to, 1-color, -eval, PICES_VALUES[piece], all);
}

int Moves::see(int to, int color, int score, int target, u64 all)
{
    u64 attacks = this->attacks_all(to, color, all) & all;
    if (attacks == 0)
        return score;

    int from = 255;
    int new_target = PICES_VALUES[Board::KING] + 1;

    while (attacks != 0)
    {
        int pos = Bitboards::poplsb(attacks);
        int piece = this->_board->_board[pos] & Board::PIECES_MASK;
        if (PICES_VALUES[piece] < new_target)
        {
            from = pos;
            new_target = PICES_VALUES[piece];
        }
    }

//    if (from >= 64)
//    {
//        std::printf("Fail :(\n");
//    }

    all ^= 1ull << from;
    int res = -this->see(to, 1-color, -(score + target), new_target, all);
    return (res > score) ? res : score;
}

void Moves::clear()
{
    this->_generated = false;
    this->_stage = STAGE_HASH;
    this->_moves_kills.clear();
    this->_moves_quiet.clear();
    this->_hist_moves.clear();
    this->_hash = 0;
    this->_hash_move = 0;
    this->_killer1_move = 0;
    this->_killer2_move = 0;
    this->_counter_move = 0;
}

bool Moves::is_legal(u16 move)
{
//    return true;

    int flags = this->_board->_nodes[this->_ply]._flags;
    int color = flags & Board::FLAG_WHITE_MOVE ? 0 : 1;

    int from = move & 63;
    int piece = this->_board->_board[from];
    if (piece == 0 || piece & (color ? Board::WHITE_MASK : Board::BLACK_MASK))
        return false;
    piece &= Board::PIECES_MASK;

    int to = (move >> 6) & 63;
    int killed_piece = this->_board->_board[to];
    if (killed_piece & (color ? Board::BLACK_MASK : Board::WHITE_MASK))
        return false;

    int killed = move & Move::KILLED;
    if (killed == 0 && killed_piece != 0)
        return false;

    Bitboards &bitboards = Bitboards::instance();
    u64 self = this->_board->_all_pieces[color];
    u64 enemies = this->_board->_all_pieces[1-color];
    u64 all = self | enemies;
    u64 moves = 0ull;

    if (piece == Board::KING)
    {
        moves |= bitboards._moves_king[from];

        if (to - from == 2)
        {
            int can_00 = color ? flags & Board::FLAG_BLACK_00 : flags & Board::FLAG_WHITE_00;
            if (can_00 && this->check_00_000(from, 1, 2, !color))
                Bitboards::bit_set(moves, to);
        }

        if (to - from == -2)
        {
            int can_000 = color ? flags & Board::FLAG_BLACK_000 : flags & Board::FLAG_WHITE_000;
            if (can_000 && this->check_00_000(from, -1, 3, !color))
                Bitboards::bit_set(moves, to);
        }
    }

    if (piece == Board::QUEEN || piece == Board::ROOK)
        moves |= bitboards._magic_rooks[from]._attacks[bitboards._magic_rooks[from].index(all)];

    if (piece == Board::QUEEN || piece == Board::BISHOP)
        moves |= bitboards._magic_bishops[from]._attacks[bitboards._magic_bishops[from].index(all)];

    if (piece == Board::KNIGHT)
        moves |= bitboards._moves_knight[from];

    if (piece == Board::PAWN)
    {
        int pawn_move = color ? -8 : 8;
        int pawn_2row = color ? 5 : 2;
        int en_passant = flags & Board::FLAG_EN_PASSANT_MASK;

        if (killed)
        {
            u64 pieces_moves = bitboards._attacks_pawn[color][from];
            moves |= pieces_moves & enemies;
            if (en_passant != 255 && Bitboards::bit_test(pieces_moves, en_passant))
                Bitboards::bit_set(moves, en_passant);
        }
        else
        {
            int new_sq = from + pawn_move;
            if (this->_board->_board[new_sq] == 0)
            {
                Bitboards::bit_set(moves, new_sq);
                if (Bitboards::_row[new_sq] == pawn_2row)
                {
                    int new_sq2 = new_sq + pawn_move;
                    if (this->_board->_board[new_sq2] == 0)
                        Bitboards::bit_set(moves, new_sq2);
                }
            }
        }
    }

    return Bitboards::bit_test(moves, to);
}

void Moves::add(int piece, int from, int to, int killed, int pawn_morph, int en_passant)
{
    int value = 0;
    pawn_morph = pawn_morph & Board::PIECES_MASK;
    u16 move = from | (to << 6) | (pawn_morph << 12);
    if (killed != 0)
        move |= Move::KILLED;

    if (killed != 0 || pawn_morph != 0)
//        value = PICES_VALUES[killed & Board::PIECES_MASK] - PICES_VALUES[piece & Board::PIECES_MASK] + PICES_VALUES[pawn_morph];
        value = this->SEE(move);
    else
    {
        int tmp1, tmp2;
        value = this->get_history((piece & Board::WHITE_MASK) ? 0 : 1, move, piece & Board::PIECES_MASK, tmp1, tmp2);
    }

//    if (!this->is_legal(move))
//    {
//        this->_board->print();
//        std::printf("Illegal move: %s\n", Move::get_string(move).c_str());
//        this->_board->test();
//    }

    if (move == this->_hash)
    {
        this->_hash_move = move;
        return;
    }
    else if (move == this->_killer1)
    {
        this->_killer1_move = move;
        return;
    }
    else if (move == this->_killer2)
    {
        this->_killer2_move = move;
        return;
    }
    else if (move == this->_counter)
    {
        this->_counter_move = move;
        return;
    }

    if (killed == 0 && pawn_morph == 0)
        this->_moves_quiet.push_back(Move(move, value));
    else
        this->_moves_kills.push_back(Move(move, value));
}

void Moves::gen_kills()
{
    int color = this->_board->color(this->_ply);

    Bitboards &bitboards = Bitboards::instance();

    u64 pieces = this->_board->_bitboards[color][Board::PAWN];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->kills_pawn(pos, color, this->_board->_nodes[this->_ply]._flags & Board::FLAG_EN_PASSANT_MASK, bitboards._attacks_pawn[color][pos]);
    }

    pieces = this->_board->_bitboards[color][Board::KNIGHT];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->kills_short(pos, color, bitboards._moves_knight[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::BISHOP] | this->_board->_bitboards[color][Board::QUEEN];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->kills_long(pos, color, bitboards._magic_bishops[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::ROOK] | this->_board->_bitboards[color][Board::QUEEN];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->kills_long(pos, color, bitboards._magic_rooks[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::KING];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->kills_short(pos, color, bitboards._moves_king[pos]);
    }
}

void Moves::gen_quiet()
{
    int color = this->_board->color(this->_ply);

    Bitboards &bitboards = Bitboards::instance();

    u64 pieces;

    pieces = this->_board->_bitboards[color][Board::KING];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->moves_king(pos, color, this->_board->_nodes[this->_ply]._flags, bitboards._moves_king[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::ROOK] | this->_board->_bitboards[color][Board::QUEEN];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->moves_long(pos, color, bitboards._magic_rooks[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::BISHOP] | this->_board->_bitboards[color][Board::QUEEN];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->moves_long(pos, color, bitboards._magic_bishops[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::KNIGHT];
    while (pieces != 0)
    {
        int pos = Bitboards::poplsb(pieces);
        this->moves_short(pos, color, bitboards._moves_knight[pos]);
    }

    pieces = this->_board->_bitboards[color][Board::PAWN];
    while (pieces != 0)
        this->moves_pawn(Bitboards::poplsb(pieces), color);
}

void Moves::kills_pawn(int square, int color, int en_passant, const u64 pieces_moves)
{
    u64 enemies = this->_board->_all_pieces[1-color];
    int pawn_move = color ? -8 : 8;
    int color_mask = color ? Board::BLACK_MASK : Board::WHITE_MASK;
    int enemy_mask = color ? Board::WHITE_MASK : Board::BLACK_MASK;
    int pawn_8row = color ? 0 : 7;

    u64 moves_mask = enemies & pieces_moves;

    // Превращения
    int new_sq = square + pawn_move;
    if (Bitboards::_row[new_sq] == pawn_8row && this->_board->_board[new_sq] == 0)
    {
        this->add(this->_board->_board[square], square, new_sq, 0, color_mask | Board::QUEEN);
        this->add(this->_board->_board[square], square, new_sq, 0, color_mask | Board::ROOK);
        this->add(this->_board->_board[square], square, new_sq, 0, color_mask | Board::BISHOP);
        this->add(this->_board->_board[square], square, new_sq, 0, color_mask | Board::KNIGHT);
    }

    // Взятия
    while (moves_mask != 0)
    {
        int new_sq = Bitboards::poplsb(moves_mask);

        // Взятие с превращением
        if (Bitboards::_row[new_sq] == pawn_8row)
        {
            this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq], color_mask | Board::QUEEN);
            this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq], color_mask | Board::ROOK);
            this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq], color_mask | Board::BISHOP);
            this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq], color_mask | Board::KNIGHT);
        }
        else
            this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq]);
    }
    // Взятие на проходе
    if (en_passant != 255 && Bitboards::bit_test(pieces_moves, en_passant))
        this->add(this->_board->_board[square], square, en_passant, enemy_mask | Board::PAWN, 0, en_passant - pawn_move);
}

void Moves::kills_short(int square, int color, const u64 pieces_moves)
{
    u64 enemies = this->_board->_all_pieces[1-color];

    u64 moves_mask = enemies & pieces_moves;
    while (moves_mask != 0)
    {
        int new_sq = Bitboards::poplsb(moves_mask);
        this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq]);
    }
}

void Moves::kills_long(int square, int color, const Magic &magic)
{
    u64 enemies = this->_board->_all_pieces[1-color];
    u64 all = this->_board->_all_pieces[color] | this->_board->_all_pieces[1-color];

    u64 attacks = magic._attacks[magic.index(all)];

    u64 moves_mask = enemies & attacks;
    while (moves_mask != 0)
    {
        int new_sq = Bitboards::poplsb(moves_mask);
        this->add(this->_board->_board[square], square, new_sq, this->_board->_board[new_sq]);
    }
}

void Moves::moves_pawn(int square, int color)
{
    int pawn_move = color ? -8 : 8;
    int pawn_2row = color ? 5 : 2;
    int pawn_8row = color ? 0 : 7;

    // Тихие ходы
    int new_sq = square + pawn_move;
    if (this->_board->_board[new_sq] == 0)
    {
        // Смотрим всё, кроме превращений
        if (Bitboards::_row[new_sq] != pawn_8row)
        {
            this->add(this->_board->_board[square], square, new_sq);
            // Проверка второго хода
            if (Bitboards::_row[new_sq] == pawn_2row)
            {
                int new_sq2 = new_sq + pawn_move;
                if (this->_board->_board[new_sq2] == 0)
                    this->add(this->_board->_board[square], square, new_sq2, 0, 0, new_sq);
            }
        }
    }
}

void Moves::moves_short(int square, int color, const u64 pieces_moves)
{
    u64 not_all = ~(this->_board->_all_pieces[color] | this->_board->_all_pieces[1-color]);

    u64 moves_mask = not_all & pieces_moves;
    while (moves_mask != 0)
    {
        int new_sq = Bitboards::poplsb(moves_mask);
        this->add(this->_board->_board[square], square, new_sq);
    }
}

void Moves::moves_long(int square, int color, const Magic &magic)
{
    u64 all = this->_board->_all_pieces[color] | this->_board->_all_pieces[1-color];

    u64 attacks = magic._attacks[magic.index(all)];

    u64 moves_mask = ~all & attacks;
    while (moves_mask != 0)
    {
        int new_sq = Bitboards::poplsb(moves_mask);
        this->add(this->_board->_board[square], square, new_sq);
    }
}

void Moves::moves_king(int square, int color, int flags, const u64 pieces_moves)
{
    bool is_white = !color;
    this->moves_short(square, color, pieces_moves);

    // Если король под атакой - нет рокировок
    if (this->_board->is_attacked(square, !is_white))
        return;

    // Короткая рокировка
    int can_00 = flags & Board::FLAG_WHITE_MOVE ? flags & Board::FLAG_WHITE_00 : flags & Board::FLAG_BLACK_00;
    if (can_00 && this->check_00_000(square, 1, 2, is_white))
        this->add(this->_board->_board[square], square, square + 2);

    // Длинная рокировка
    int can_000 = flags & Board::FLAG_WHITE_MOVE ? flags & Board::FLAG_WHITE_000 : flags & Board::FLAG_BLACK_000;
    if (can_000 && this->check_00_000(square, -1, 3, is_white))
        this->add(this->_board->_board[square],  square, square - 2);
}

bool Moves::check_00_000(int square, int piece_move, int count_empty, bool is_white)
{
    int new_sq = square;
    // Проверка свободных полей
    for (int i = 0; i < count_empty; ++i)
    {
        new_sq += piece_move;
        if (this->_board->_board[new_sq])
            return false;
    }
    // проверка атакованности полей
    new_sq = square;
    for (int i = 0; i < 2; ++i)
    {
        new_sq += piece_move;
        if (this->_board->is_attacked(new_sq, !is_white))
            return false;
    }
    return true;
}

void Moves::history_bonus(int &node, int bonus)
{
    int abs_bonus = bonus > 0 ? bonus : -bonus;
    node += 32 * bonus - node * abs_bonus / 512;
}

u64 Moves::attacks_all(int pos, int color, u64 all)
{
    Bitboards &bitboards = Bitboards::instance();

    u64 res = 0;

    res |= bitboards._attacks_pawn[1-color][pos] & this->_board->_bitboards[color][Board::PAWN];    // BB_PAWN_ATTACKS[to][side ^ 1] & Bits(PW | side);
    res |= bitboards._moves_knight[pos] & this->_board->_bitboards[color][Board::KNIGHT];           // BB_KNIGHT_ATTACKS[to] & Bits(NW | side);
    res |= bitboards._moves_king[pos] & this->_board->_bitboards[color][Board::KING];               // BB_KING_ATTACKS[to] & Bits(KW | side);
    res |= bitboards._magic_bishops[pos]._attacks[bitboards._magic_bishops[pos].index(all)] &
            (this->_board->_bitboards[color][Board::BISHOP] | this->_board->_bitboards[color][Board::QUEEN]);   // BishopAttacks(to, occ) & (Bits(BW | side) | Bits(QW | side));
    res |= bitboards._magic_rooks[pos]._attacks[bitboards._magic_rooks[pos].index(all)] &
            (this->_board->_bitboards[color][Board::ROOK] | this->_board->_bitboards[color][Board::QUEEN]);   // RookAttacks(to, occ) & (Bits(RW | side) | Bits(QW | side));

    return res;
}
