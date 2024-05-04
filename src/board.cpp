#include "board.h"

#include "hash.h"
#include "bitboards.h"

#include <cstdio>
#include <map>

static const std::vector<char> &SYMBOLS = { '.', 'K', 'Q', 'R', 'B', 'N', 'P', '-',
                                            '_', 'k', 'q', 'r', 'b', 'n', 'p', '+' };

static const std::map<char, int> &PIECES = { {'K', Board::WHITE_MASK | Board::KING},
                                             {'Q', Board::WHITE_MASK | Board::QUEEN},
                                             {'R', Board::WHITE_MASK | Board::ROOK},
                                             {'B', Board::WHITE_MASK | Board::BISHOP},
                                             {'N', Board::WHITE_MASK | Board::KNIGHT},
                                             {'P', Board::WHITE_MASK | Board::PAWN},
                                             {'k', Board::BLACK_MASK | Board::KING},
                                             {'q', Board::BLACK_MASK | Board::QUEEN},
                                             {'r', Board::BLACK_MASK | Board::ROOK},
                                             {'b', Board::BLACK_MASK | Board::BISHOP},
                                             {'n', Board::BLACK_MASK | Board::KNIGHT},
                                             {'p', Board::BLACK_MASK | Board::PAWN} };

static const std::string &START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


static const std::vector<int> &WHITE_PAWN_ATTACKS = { 15, 17 };
static const std::vector<int> &BLACK_PAWN_ATTACKS = { -15, -17 };
static const std::vector<int> &KNIGHT_MOVES = { -14, 14, -18, 18, -31, 31, -33, 33 };
static const std::vector<int> &BISHOP_MOVES = { -15, 15, -17, 17 };
static const std::vector<int> &ROOK_MOVES = { -1, 1, -16, 16 };
static const std::vector<int> &QUEEN_MOVES = { -1, 1, -16, 16,  -15, 15, -17, 17 };


std::string Node::get_pv(int max)
{
    std::string res = "";

    for (int i = 0; i < this->_pv[0]; ++i)
    {
        if (i == max)
            break;
        res += (i == 0 ? "" : " ") + Move::get_string(this->_pv[1+i]);
    }

    return res;
}

Board::Board()
{
    this->_moves_cnt = 0;
    for (int i = 0; i < 150; ++i)
        this->_moves[i].init(this, &this->_history);
    this->reset();
}

std::string Board::get_start_fen()
{
    return START_FEN;
}

void Board::reset()
{
    this->set_fen(START_FEN);
}

void Board::print()
{
    for (int i = 7; i >= 0; --i)
    {
        for (int j = 0; j < 8; ++j)
        {
            int sq = i*8 + j;
            std::printf("%c", SYMBOLS[this->_board[sq] & (Board::BLACK_MASK | Board::PIECES_MASK)]);
        }
        std::printf("\n");
    }
    std::printf(this->is_white(0) ? "White\n\n": "Black\n\n");
}

void Board::set_fen(const std::string &fen)
{
    this->_positions.clear();

    for (int i = 0; i < 7; ++i)
    {
        this->_bitboards[0][i] = 0;
        this->_bitboards[1][i] = 0;
    }
    this->_all_pieces[0] = 0;
    this->_all_pieces[1] = 0;

    int col = 0,
        row = 7,
        pos = 0;
    char c = ' ';

    this->_wking = 255;
    this->_bking = 255;
    this->_nodes[0]._flags = 0;
    this->_nodes[0]._hash = 0;

    Zorbist &zorb = Zorbist::instance();

    // Фигуры
    while (true)
    {
        c = fen[pos++];
        if (c == ' ')
            break;
        if (c == '/')
        {
            col = 0;
            row--;
            continue;
        }
        if (c > '0' && c < '9')
        {
            int cnt = c - '0';
            for (int i = 0; i < cnt; ++i)
                this->_board[row*8 + col++] = 0;
            continue;
        }
        int sq = row*8 + col++;
        if (c == 'K')
            this->_wking = sq;
        if (c == 'k')
            this->_bking = sq;

        int piece = PIECES.at(c);
        this->piece_add(-1, piece & Board::WHITE_MASK ? 0 : 1, piece, sq, false);
    }

    // Чей ход
    c = fen[pos++];
    if (c == 'w')
    {
        this->_nodes[0]._flags |= FLAG_WHITE_MOVE;
        this->_nodes[0]._hash ^= zorb._white_move;
    }
    pos++;

    // Рокировки
    while (true)
    {
        c = fen[pos++];
        if (c == ' ')
            break;
        else if (c == 'K')
        {
            this->_nodes[0]._flags |= FLAG_WHITE_00;
            this->_nodes[0]._hash ^= zorb._white_00;
        }
        else if (c == 'Q')
        {
            this->_nodes[0]._flags |= FLAG_WHITE_000;
            this->_nodes[0]._hash ^= zorb._white_000;
        }
        else if (c == 'k')
        {
            this->_nodes[0]._flags |= FLAG_BLACK_00;
            this->_nodes[0]._hash ^= zorb._black_00;
        }
        else if (c == 'q')
        {
            this->_nodes[0]._flags |= FLAG_BLACK_000;
            this->_nodes[0]._hash ^= zorb._black_000;
        }
    }

    // Взятие на проходе (битое поле)
    // position startpos moves h2h4 f7f5 h4h5 g7g5
    // position fen rnbqkbnr/ppppp2p/8/5ppP/8/8/PPPPPPP1/RNBQKBNR w KQkq g6 0 3
    c = fen[pos++];
    if (c != '-')
    {
        col = c - 'a';
        c = fen[pos++];
        row = c - '1';
        int sq = row*8 + col;
        this->_nodes[0]._flags |= sq;
        this->_nodes[0]._hash ^= zorb._en_passant[sq];
    }
    else
        this->_nodes[0]._flags |= FLAG_EN_PASSANT_MASK;
    pos++;

    // Число полуходов после последнего хода пешки или взятия

    // Номер хода (инкрементируется после хода черных)

    // Для правила 50 ходов. По хорошему, нужно брать из fen, но так лень )
    this->_dtz_current = 0;
    this->_dtz[0] = 100;

    // Нейронки
    this->_stack_pointer = 0;
#ifdef USE_NN
    this->_neural.accum_all_pieces(this->_stack_pointer,
                                   this->_bitboards[0][Board::KING],
                                   this->_bitboards[1][Board::KING],
                                   this->_bitboards[0][Board::PAWN],
                                   this->_bitboards[1][Board::PAWN],
                                   this->_bitboards[0][Board::KNIGHT],
                                   this->_bitboards[1][Board::KNIGHT],
                                   this->_bitboards[0][Board::BISHOP],
                                   this->_bitboards[1][Board::BISHOP],
                                   this->_bitboards[0][Board::ROOK],
                                   this->_bitboards[1][Board::ROOK],
                                   this->_bitboards[0][Board::QUEEN],
                                   this->_bitboards[1][Board::QUEEN]);
    this->_wking_area = Neural::king_area(this->_wking, 0);
    this->_bking_area = Neural::king_area(this->_bking, 1);
#endif

    // Добавляем позицию
    this->add_position();
}

std::string Board::get_fen(int ply)
{
    std::string result = "";

    for (int i = 7; i >= 0; --i)
    {
        int spaces = 0;
        for (int j = 0; j < 8; ++j)
        {
            int sq = i*8 + j;
            if (this->_board[sq] == 0)
            {
                spaces++;
                continue;
            }
            if (spaces > 0)
                result += char('0' + spaces);
            spaces = 0;
            result += SYMBOLS[this->_board[sq] & (Board::BLACK_MASK | Board::PIECES_MASK)];
        }
        if (spaces > 0)
            result += char('0' + spaces);
        if (i > 0)
            result += "/";
    }

    result += this->color(ply) == 0 ? " w " : " b ";

    result += this->_nodes[ply]._flags & FLAG_WHITE_00 ? "K" : "";
    result += this->_nodes[ply]._flags & FLAG_WHITE_000 ? "Q" : "";
    result += this->_nodes[ply]._flags & FLAG_BLACK_00 ? "k" : "";
    result += this->_nodes[ply]._flags & FLAG_BLACK_000 ? "q" : "";

    result += " -";

    return result;
}

bool Board::is_attacked(int square, bool is_white)
{
    Bitboards &bitboards = Bitboards::instance();
    int color = is_white ? 0 : 1;
    u64 all = this->_all_pieces[0] | this->_all_pieces[1];

    if (bitboards._attacks_pawn[1-color][square] & this->_bitboards[color][Board::PAWN])
        return true;
    if (bitboards._moves_knight[square] & this->_bitboards[color][Board::KNIGHT])
        return true;
    if (bitboards._magic_bishops[square]._attacks[bitboards._magic_bishops[square].index(all)] & (this->_bitboards[color][Board::QUEEN] | this->_bitboards[color][Board::BISHOP]))
        return true;
    if (bitboards._magic_rooks[square]._attacks[bitboards._magic_rooks[square].index(all)] & (this->_bitboards[color][Board::QUEEN] | this->_bitboards[color][Board::ROOK]))
        return true;
    if (bitboards._moves_king[square] & this->_bitboards[color][Board::KING])
        return true;

    return false;
}

Moves* Board::moves_init(int ply, bool kills)
{
    u16 move_counter = 0;
    int piece_counter = 0;
    if (ply >= 1)
    {
        move_counter = this->_nodes[ply-1]._move;
        piece_counter = this->_nodes[ply-1]._move_piece & Board::PIECES_MASK;
    }
    u16 move_follower = 0;
    int piece_follower = 0;
    if (ply >= 2 && move_counter != 0)
    {
        move_follower = this->_nodes[ply-2]._move;
        piece_follower = this->_nodes[ply-2]._move_piece & Board::PIECES_MASK;
    }

    this->_moves[this->_moves_cnt].init_generator(kills, ply, move_counter, piece_counter, move_follower, piece_follower);
    this->_moves[this->_moves_cnt+1].clear_killers();
    return &this->_moves[this->_moves_cnt++];
}

void Board::moves_free()
{
    this->_moves_cnt--;
}

bool Board::move_do(u16 move, int ply, bool same_ply)
{
    Zorbist &zorb = Zorbist::instance();

    int color = this->_nodes[ply]._flags & FLAG_WHITE_MOVE ? 0 : 1;

    int from = move & 63;
    int to = (move >> 6) & 63;

    int pawn_morph = (move >> 12) & 7;
    if (pawn_morph != 0)
        pawn_morph |= ((color == 0) ? Board::WHITE_MASK : Board::BLACK_MASK);
    int killed = move & Move::KILLED;
    int en_passant = FLAG_EN_PASSANT_MASK;

    int piece = this->_board[from];

    if (killed != 0)
    {
        killed = this->_board[to];
        if (killed == 0)
        {
            killed = Board::PAWN | ((color == 0) ? Board::BLACK_MASK : Board::WHITE_MASK);
            en_passant = to + ((color == 0) ? -8 : 8);
        }
    }

    this->_nodes[ply+1]._flags = this->_nodes[ply]._flags;
    this->_nodes[ply+1]._hash = this->_nodes[ply]._hash;

    bool king_move = (piece & Board::PIECES_MASK) == Board::KING;
    bool nn_update = true;

    this->_stack_pointer++;
#ifdef USE_NN
    if (king_move)
    {
//        nn_update = false;
        int new_king_area = Neural::king_area(to, color);
        if (color == 0 && this->_wking_area != new_king_area)
        {
            this->_wking_area = new_king_area;
            nn_update = false;
        }
        if (color != 0 && this->_bking_area != new_king_area)
        {
            this->_bking_area = new_king_area;
            nn_update = false;
        }
    }
    if (nn_update)
        this->_neural.accum_copy(this->_stack_pointer-1, this->_stack_pointer);
#endif

    // Убираем взятого
    if (killed != 0 && en_passant == FLAG_EN_PASSANT_MASK)
        this->piece_remove(ply, 1-color, killed, to, nn_update);

    // Двигаем фигуру на доске
    this->piece_remove(ply, color, piece, from, nn_update);
    if (pawn_morph == 0)
        this->piece_add(ply, color, piece, to, nn_update);

    // Сбрасываем флаг взятия на проходе
    if ((this->_nodes[ply+1]._flags & FLAG_EN_PASSANT_MASK) != FLAG_EN_PASSANT_MASK)
        this->_nodes[ply+1]._hash ^= zorb._en_passant[this->_nodes[ply+1]._flags & FLAG_EN_PASSANT_MASK];
    this->_nodes[ply+1]._flags |= FLAG_EN_PASSANT_MASK;

    // Если прибили ладью - сбрасываем флаг соответствующей рокировки
    if ((killed & Board::PIECES_MASK) == Board::ROOK)
    {
        if (to == 7)
        {
            if (this->_nodes[ply+1]._flags & FLAG_WHITE_00)
            {
                this->_nodes[ply+1]._hash ^= zorb._white_00;
                this->_nodes[ply+1]._flags &= ~FLAG_WHITE_00;
            }
        }
        if (to == 0)
        {
            if (this->_nodes[ply+1]._flags & FLAG_WHITE_000)
            {
                this->_nodes[ply+1]._hash ^= zorb._white_000;
                this->_nodes[ply+1]._flags &= ~FLAG_WHITE_000;
            }
        }
        if (to == 63)
        {
            if (this->_nodes[ply+1]._flags & FLAG_BLACK_00)
            {
                this->_nodes[ply+1]._hash ^= zorb._black_00;
                this->_nodes[ply+1]._flags &= ~FLAG_BLACK_00;
            }
        }
        if (to == 56)
        {
            if (this->_nodes[ply+1]._flags & FLAG_BLACK_000)
            {
                this->_nodes[ply+1]._hash ^= zorb._black_000;
                this->_nodes[ply+1]._flags &= ~FLAG_BLACK_000;
            }
        }
    }

    // Особенности хода короля
    if (king_move)
    {
        if (this->_nodes[ply+1]._flags & FLAG_WHITE_MOVE)
        {
            this->_wking = to;
            if (this->_nodes[ply+1]._flags & FLAG_WHITE_00)
            {
                this->_nodes[ply+1]._hash ^= zorb._white_00;
                this->_nodes[ply+1]._flags &= ~FLAG_WHITE_00;
            }
            if (this->_nodes[ply+1]._flags & FLAG_WHITE_000)
            {
                this->_nodes[ply+1]._hash ^= zorb._white_000;
                this->_nodes[ply+1]._flags &= ~FLAG_WHITE_000;
            }
        }
        else
        {
            this->_bking = to;
            if (this->_nodes[ply+1]._flags & FLAG_BLACK_00)
            {
                this->_nodes[ply+1]._hash ^= zorb._black_00;
                this->_nodes[ply+1]._flags &= ~FLAG_BLACK_00;
            }
            if (this->_nodes[ply+1]._flags & FLAG_BLACK_000)
            {
                this->_nodes[ply+1]._hash ^= zorb._black_000;
                this->_nodes[ply+1]._flags &= ~FLAG_BLACK_000;
            }
        }

        // Если одна из рокировок, двигаем ладью
        // 00
        if (to - from == 2)
        {
            int rook = this->_board[to + 1];
            this->piece_remove(ply, color, rook, to + 1, nn_update);
            this->piece_add(ply, color, rook, to - 1, nn_update);
        }
        // 000
        if (to - from == -2)
        {
            int rook = this->_board[to - 2];
            this->piece_remove(ply, color, rook, to - 2, nn_update);
            this->piece_add(ply, color, rook, to + 1, nn_update);
        }
    }

    // Особенности хода ладьи
    if ((piece & Board::PIECES_MASK) == Board::ROOK)
    {
        if (this->_nodes[ply+1]._flags & FLAG_WHITE_MOVE)
        {
            if (from == 7)
            {
                if (this->_nodes[ply+1]._flags & FLAG_WHITE_00)
                {
                    this->_nodes[ply+1]._hash ^= zorb._white_00;
                    this->_nodes[ply+1]._flags &= ~FLAG_WHITE_00;
                }
            }
            if (from == 0)
            {
                if (this->_nodes[ply+1]._flags & FLAG_WHITE_000)
                {
                    this->_nodes[ply+1]._hash ^= zorb._white_000;
                    this->_nodes[ply+1]._flags &= ~FLAG_WHITE_000;
                }
            }
        }
        else
        {
            if (from == 63)
            {
                if (this->_nodes[ply+1]._flags & FLAG_BLACK_00)
                {
                    this->_nodes[ply+1]._hash ^= zorb._black_00;
                    this->_nodes[ply+1]._flags &= ~FLAG_BLACK_00;
                }
            }
            if (from == 56)
            {
                if (this->_nodes[ply+1]._flags & FLAG_BLACK_000)
                {
                    this->_nodes[ply+1]._hash ^= zorb._black_000;
                    this->_nodes[ply+1]._flags &= ~FLAG_BLACK_000;
                }
            }
        }
    }

    // Особенности хода пешки
    if ((piece & Board::PIECES_MASK) == Board::PAWN)
    {
        if (pawn_morph)
            this->piece_add(ply, color, pawn_morph, to, true);

        if (to - from == 16)
        {
            this->_nodes[ply+1]._flags &= ~FLAG_EN_PASSANT_MASK;
            this->_nodes[ply+1]._flags |= from + 8;
            this->_nodes[ply+1]._hash ^= zorb._en_passant[from + 8];
            en_passant = to;
        }
        if (to - from == -16)
        {
            this->_nodes[ply+1]._flags &= ~FLAG_EN_PASSANT_MASK;
            this->_nodes[ply+1]._flags |= from - 8;
            this->_nodes[ply+1]._hash ^= zorb._en_passant[from - 8];
            en_passant = to;
        }

        if (en_passant != FLAG_EN_PASSANT_MASK && killed != 0)
            this->piece_remove(ply, 1-color, killed, en_passant, true);
    }

    this->_nodes[ply]._move = move;
    this->_nodes[ply]._move_piece = piece;
    this->_nodes[ply]._move_killed = killed;
    this->_nodes[ply]._move_en_passant = en_passant;

    this->_dtz[this->_dtz_current + 1] = this->_dtz[this->_dtz_current] - 1;
    this->_dtz_current++;
    if (((piece & Board::PIECES_MASK) == Board::PAWN) || killed != 0)
        this->_dtz[this->_dtz_current] = 100;

    // Если после хода шах ходившей стороне, то отменяем ход, он некорректный
    if (this->is_check(ply))
    {
        this->move_undo(move, ply);
        return false;
    }

#ifdef USE_NN
    if (!nn_update)
        this->_neural.accum_all_pieces(this->_stack_pointer,
                                       this->_bitboards[0][Board::KING],
                                       this->_bitboards[1][Board::KING],
                                       this->_bitboards[0][Board::PAWN],
                                       this->_bitboards[1][Board::PAWN],
                                       this->_bitboards[0][Board::KNIGHT],
                                       this->_bitboards[1][Board::KNIGHT],
                                       this->_bitboards[0][Board::BISHOP],
                                       this->_bitboards[1][Board::BISHOP],
                                       this->_bitboards[0][Board::ROOK],
                                       this->_bitboards[1][Board::ROOK],
                                       this->_bitboards[0][Board::QUEEN],
                                       this->_bitboards[1][Board::QUEEN]);
#endif

    // Меняем цвет
    this->_nodes[ply+1]._flags ^= FLAG_WHITE_MOVE;
    this->_nodes[ply+1]._hash ^= zorb._white_move;

    TranspositionTable &table = TranspositionTable::instance();
    table.prefetch(this->_nodes[ply+1]._hash);

    if (same_ply)
    {
        this->_nodes[ply]._flags = this->_nodes[ply+1]._flags;
        this->_nodes[ply]._hash = this->_nodes[ply+1]._hash;
        this->_stack_pointer--;
#ifdef USE_NN
        this->_neural.accum_copy(this->_stack_pointer+1, this->_stack_pointer);
#endif
    }

    return true;
}

void Board::move_undo(u16 move, int ply)
{
    int color = this->_nodes[ply]._flags & FLAG_WHITE_MOVE ? 0 : 1;

    int from = move & 63;
    int to = (move >> 6) & 63;

    int pawn_morph = (move >> 12) & 7 | ((color == 0) ? Board::WHITE_MASK : Board::BLACK_MASK);

    int piece = this->_nodes[ply]._move_piece;
    int killed = this->_nodes[ply]._move_killed;
    int en_passant = this->_nodes[ply]._move_en_passant;

    this->_stack_pointer--;
    this->_dtz_current--;

    // Перемещаем фигуру обратно. Если кого-то рубили, возвращаем и его
    this->_board[to] = killed;
    this->_board[from] = piece;

    Bitboards::bit_clear(this->_bitboards[color][piece & Board::PIECES_MASK], to);
    Bitboards::bit_clear(this->_all_pieces[color], to);
    Bitboards::bit_set(this->_bitboards[color][piece & Board::PIECES_MASK], from);
    Bitboards::bit_set(this->_all_pieces[color], from);
    if (killed != 0)
    {
        Bitboards::bit_set(this->_bitboards[1-color][killed & Board::PIECES_MASK], to);
        Bitboards::bit_set(this->_all_pieces[1-color], to);
    }

    // Особенности хода пешки
    if ((piece & Board::PIECES_MASK) == Board::PAWN)
    {
        if (en_passant != FLAG_EN_PASSANT_MASK && killed != 0)
        {
            this->_board[en_passant] = killed;
            this->_board[to] = 0;
            Bitboards::bit_set(this->_bitboards[1-color][Board::PAWN], en_passant);
            Bitboards::bit_set(this->_all_pieces[1-color], en_passant);
            Bitboards::bit_clear(this->_bitboards[1-color][Board::PAWN], to);
            Bitboards::bit_clear(this->_all_pieces[1-color], to);
        }
        if (pawn_morph)
            Bitboards::bit_clear(this->_bitboards[color][pawn_morph & Board::PIECES_MASK], to);
    }

    // Особенности хода короля
    if ((piece & Board::PIECES_MASK) == Board::KING)
    {
        if (this->_nodes[ply]._flags & FLAG_WHITE_MOVE)
        {
            this->_wking = from;
            this->_wking_area = Neural::king_area(from, 0);
        }
        else
        {
            this->_bking = from;
            this->_bking_area = Neural::king_area(from, 1);
        }

        // Если одна из рокировок, двигаем ладью
        // 00
        if (to - from == 2)
        {
            this->_board[to + 1] = this->_board[to - 1];
            this->_board[to - 1] = 0;
            Bitboards::bit_set(this->_bitboards[color][Board::ROOK], to+1);
            Bitboards::bit_set(this->_all_pieces[color], to+1);
            Bitboards::bit_clear(this->_bitboards[color][Board::ROOK], to-1);
            Bitboards::bit_clear(this->_all_pieces[color], to-1);
        }
        // 000
        if (to - from == -2)
        {
            this->_board[to - 2] = this->_board[to + 1];
            this->_board[to + 1] = 0;
            Bitboards::bit_set(this->_bitboards[color][Board::ROOK], to-2);
            Bitboards::bit_set(this->_all_pieces[color], to-2);
            Bitboards::bit_clear(this->_bitboards[color][Board::ROOK], to+1);
            Bitboards::bit_clear(this->_all_pieces[color], to+1);
        }
    }
}

void Board::nullmove_do(int ply)
{
    Zorbist &zorb = Zorbist::instance();

    this->_nodes[ply+1]._flags = this->_nodes[ply]._flags ^ FLAG_WHITE_MOVE;
    this->_nodes[ply+1]._hash = this->_nodes[ply]._hash ^ zorb._white_move;
//    this->_nodes[ply+1]._pk_neural = this->_nodes[ply]._pk_neural;

    if ((this->_nodes[ply+1]._flags & FLAG_EN_PASSANT_MASK) != FLAG_EN_PASSANT_MASK)
        this->_nodes[ply+1]._hash ^= zorb._en_passant[this->_nodes[ply+1]._flags & FLAG_EN_PASSANT_MASK];
    this->_nodes[ply+1]._flags |= FLAG_EN_PASSANT_MASK;

    this->_nodes[ply]._move = 0;

    this->_moves_cnt++;

    TranspositionTable &table = TranspositionTable::instance();
    table.prefetch(this->_nodes[ply+1]._hash);
}

void Board::nullmove_undo(int ply)
{
    this->_moves_cnt--;
}

bool Board::is_white(int ply)
{
    return this->_nodes[ply]._flags & FLAG_WHITE_MOVE;
}

int Board::color(int ply)
{
    return this->_nodes[ply]._flags & FLAG_WHITE_MOVE ? 0 : 1;
}

bool Board::is_check(int ply)
{
    if (this->_nodes[ply]._flags & FLAG_WHITE_MOVE)
        return this->is_attacked(this->_wking, false);
    else
        return this->is_attacked(this->_bking, true);
}

bool Board::is_enemy_check(int ply)
{
    if (this->_nodes[ply]._flags & FLAG_WHITE_MOVE)
        return this->is_attacked(this->_bking, true);
    else
        return this->is_attacked(this->_wking, false);
}

void Board::add_position()
{
    u64 hash = this->_nodes[0]._hash;
    if (this->_positions.find(hash) == this->_positions.end())
        this->_positions.insert(std::make_pair(hash, 1));
    else
        this->_positions[hash]++;
}

bool Board::check_draw(int ply, int triple_moves_count)
{
    // Правило 50 ходов
    if (this->_dtz[this->_dtz_current] <= 0)
        return true;

    // Поврторение позиции
    u64 hash = this->_nodes[ply]._hash;
    int count = (this->_positions.find(hash) == this->_positions.end()) ? 0 : this->_positions[hash];
    for (int i = 1; i <= ply; ++i)
        if (this->_nodes[i]._hash == hash)
            count++;
    if (count >= triple_moves_count)
        return true;

    // Есть пешки или тяжелые фигуры - точно не ничья
    if (this->_bitboards[0][Board::PAWN] || this->_bitboards[1][Board::PAWN] ||
            this->_bitboards[0][Board::ROOK] || this->_bitboards[1][Board::ROOK] ||
            this->_bitboards[0][Board::QUEEN] || this->_bitboards[1][Board::QUEEN])
        return false;
    // Король против короля
    if (this->_all_pieces[0] == this->_bitboards[0][Board::KING] &&
            this->_all_pieces[1] == this->_bitboards[1][Board::KING])
        return true;

    u64 knights = this->_bitboards[0][Board::KNIGHT] | this->_bitboards[1][Board::KNIGHT];
    u64 bishops = this->_bitboards[0][Board::BISHOP] | this->_bitboards[1][Board::BISHOP];
    // Король против коня или слона
    // Конь или король протик коня
    return !Bitboards::several(knights | bishops) || (!bishops && Bitboards::bits_count(knights) <= 2);
}

int Board::get_dtz()
{
    return this->_dtz[this->_dtz_current];
}

u64 Board::get_hash(int ply)
{
    return this->_nodes[ply]._hash;
}

int Board::get_max_figure_value(int color)
{
    int res = 100;
    if (this->_bitboards[1-color][Board::QUEEN] != 0)
        res = 1000;
    else if (this->_bitboards[1-color][Board::ROOK] != 0)
        res = 500;
    else if (this->_bitboards[1-color][Board::BISHOP]  != 0 || this->_bitboards[1-color][Board::KNIGHT] != 0)
        res = 300;

    if (this->_bitboards[color][Board::PAWN] & (color ? Bitboards::_rows[1] : Bitboards::_rows[6]))
        res += 1000 - 100;

    return res;
}

bool Board::is_figures(int color)
{
    return this->_bitboards[color][Board::KNIGHT] != 0 || this->_bitboards[color][Board::BISHOP]  != 0 ||
            this->_bitboards[color][Board::ROOK] != 0 || this->_bitboards[color][Board::QUEEN] != 0;
}

void Board::piece_remove(int ply, int color, int piece, int square, bool nn)
{
    Zorbist &zorb = Zorbist::instance();
    Bitboards::bit_clear(this->_bitboards[color][piece & Board::PIECES_MASK], square);
    Bitboards::bit_clear(this->_all_pieces[color], square);
    this->_nodes[ply+1]._hash ^= zorb._pieces[piece][square];
    this->_board[square] = 0;
#ifdef USE_NN
    if (nn)
        this->_neural.accum_piece_remove(this->_stack_pointer, this->_wking, this->_bking, color, piece & Board::PIECES_MASK, square);
#endif
}

void Board::piece_add(int ply, int color, int piece, int square, bool nn)
{
    Zorbist &zorb = Zorbist::instance();
    Bitboards::bit_set(this->_bitboards[color][piece & Board::PIECES_MASK], square);
    Bitboards::bit_set(this->_all_pieces[color], square);
    this->_nodes[ply+1]._hash ^= zorb._pieces[piece][square];
    this->_board[square] = piece;
#ifdef USE_NN
    if (nn)
        this->_neural.accum_piece_add(this->_stack_pointer, this->_wking, this->_bking, color, piece & Board::PIECES_MASK, square);
#endif
}
