#include "bitboards.h"

#include <cstdio>
#include <vector>
#include <immintrin.h>

static const u64 MAGIC_ROOK[64] = {
    0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
    0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
    0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
    0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
    0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
    0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
    0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
    0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
    0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
    0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
    0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
    0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
    0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
    0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
    0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
    0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull
};

static const u64 MAGIC_BISHOP[64] = {
    0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
    0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
    0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
    0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
    0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
    0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
    0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
    0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
    0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
    0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
    0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
    0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
    0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
    0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
    0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
    0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull
};

static const std::vector<int> &WHITE_PAWN_ATTACKS = { 15, 17 };
static const std::vector<int> &BLACK_PAWN_ATTACKS = { -15, -17 };
static const std::vector<int> &KNIGHT_MOVES = { -14, 14, -18, 18, -31, 31, -33, 33 };
static const std::vector<int> &BISHOP_MOVES = { -15, 15, -17, 17 };
static const std::vector<int> &ROOK_MOVES = { -1, 1, -16, 16 };
static const std::vector<int> &QUEEN_MOVES = { -1, 1, -16, 16,  -15, 15, -17, 17 };


const u64 Bitboards::_cols[8] = {
    0x0101010101010101ull,
    0x0202020202020202ull,
    0x0404040404040404ull,
    0x0808080808080808ull,
    0x1010101010101010ull,
    0x2020202020202020ull,
    0x4040404040404040ull,
    0x8080808080808080ull
};

const u64 Bitboards::_rows[8] = {
    0x00000000000000FFull,
    0x000000000000FF00ull,
    0x0000000000FF0000ull,
    0x00000000FF000000ull,
    0x000000FF00000000ull,
    0x0000FF0000000000ull,
    0x00FF000000000000ull,
    0xFF00000000000000ull
};

const u8 Bitboards::_col[64] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7
};

const u8 Bitboards::_row[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7
};

const u64 Bitboards::_darks = 0xAA55AA55AA55AA55ull;
const u64 Bitboards::_lights = 0x55AA55AA55AA55AAull;

#ifndef USE_POPCNT
static int _popcnt[65536];
#endif

Magic::Magic()
{
    this->_magic = 0ull;
    this->_mask = 0ull;
    this->_shift = 0ull;
    for (int i = 0; i < 4096; ++i)
        this->_attacks[i] = 0xFFFFFFFFFFFFFFFFull;
}

int Magic::index(u64 pieces) const
{
#ifdef USE_PEXT
    return _pext_u64(pieces, this->_mask);
#else
    return ((pieces & this->_mask) * this->_magic) >> this->_shift;
#endif
}


Bitboards &Bitboards::instance()
{
    static Bitboards theSingleInstance;
    return theSingleInstance;
}

void Bitboards::print(const u64 &bitboard)
{
    for (u8 i = 0; i < 8; ++i)
    {
        u8 row = static_cast<u8>((bitboard >> (7 - i) * 8) & 0xFF);
        for (u8 j = 0; j < 8; ++j)
            if ((row >> j) & 1)
                printf("*");
            else
                printf(".");
        printf("\n");
    }
    printf("\n");
}

int Bitboards::lsb(const u64 bitboard)
{
    return __builtin_ctzll(bitboard);
//    return _tzcnt_u64(bitboard);
}

int Bitboards::msb(const u64 bitboard)
{
    return __builtin_clzll(bitboard) ^ 63;
//    return _lzcnt_u64(bitboard) ^ 63;
}

int Bitboards::poplsb(u64 &bitboard)
{
    int result = __builtin_ctzll(bitboard);
//    int result = _tzcnt_u64(bitboard);
    bitboard &= ~(1ull << result);
    return result;
}

int Bitboards::bits_count(const u64 bitboard)
{
#ifdef USE_POPCNT
    return __builtin_popcountll(bitboard);
//    return _mm_popcnt_u64(bitboard);
#else
    union
    {
        u64 full;
        u16 q[4];
    } u = { bitboard };
    return _popcnt[u.q[0]] + _popcnt[u.q[1]] + _popcnt[u.q[2]] + _popcnt[u.q[3]];
#endif
}

void Bitboards::bit_set(u64 &bitboard, int bit)
{
    bitboard |= (1ull << bit);
}

void Bitboards::bit_clear(u64 &bitboard, int bit)
{
    bitboard &= ~(1ull << bit);
}

bool Bitboards::bit_test(const u64 bitboard, int bit)
{
    return bitboard & (1ull << bit);
}

bool Bitboards::several(const u64 bitboard)
{
    return bitboard & (bitboard - 1);
}

u64 Bitboards::pawns_moves(const u64 pawns, const u64 all, int color)
{
    return ~all & (color ? (pawns >> 8) : (pawns << 8));
}

Bitboards::Bitboards()
{
    this->init();
}

void Bitboards::init()
{
#ifndef USE_POPCNT
    for (int i = 0; i < 65536; ++i)
    {
        int count = 0;
        int num = i;
        while (num > 0)
        {
            if ((num % 2) == 1)
                count++;
            num /= 2;
        }
        _popcnt[i] = count;
    }
#endif

    this->_board_left = Bitboards::_cols[0] | Bitboards::_cols[1] | Bitboards::_cols[2] | Bitboards::_cols[3];
    this->_board_right = Bitboards::_cols[4] | Bitboards::_cols[5] | Bitboards::_cols[6] | Bitboards::_cols[7];

    bool is_dark = true;
    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            this->_dark[i*8 + j] = is_dark;
            is_dark = !is_dark;
        }
        is_dark = !is_dark;
    }

    for (int sq = 0; sq < 64; ++sq)
    {
        this->_moves_king[sq] = moves_short(sq, QUEEN_MOVES);
        this->_moves_knight[sq] = moves_short(sq, KNIGHT_MOVES);
        this->_attacks_pawn[0][sq] = moves_short(sq, WHITE_PAWN_ATTACKS);
        this->_attacks_pawn[1][sq] = moves_short(sq, BLACK_PAWN_ATTACKS);

        this->_attacks_rooks[sq] = this->attacks_long(sq, 0ull, ROOK_MOVES);
        this->_attacks_bishops[sq] = this->attacks_long(sq, 0ull, BISHOP_MOVES);

        this->_cols_forward[0][sq] = this->init_cols_forward(sq, 8);
        this->_cols_forward[1][sq] = this->init_cols_forward(sq, -8);

        this->init_magic(sq, this->_magic_rooks[sq], MAGIC_ROOK[sq], ROOK_MOVES);
        this->init_magic(sq, this->_magic_bishops[sq], MAGIC_BISHOP[sq], BISHOP_MOVES);
    }

    for (int sq = 0; sq < 64; ++sq)
    {
        this->_passed[0][sq] = this->init_passed(0, sq);
        this->_passed[1][sq] = this->init_passed(1, sq);
    }

    for (int col = 0; col < 8; ++col)
        this->_isolated[col] = this->init_isolated(col);
}

u64 Bitboards::moves_short(int square, const std::vector<int> &pieces_moves)
{
    u64 result = 0ull;
    int sq = ((square & 56) << 1) + (square & 7);

    for (auto &move: pieces_moves)
    {
        int new_sq = sq + move;
        if (new_sq & 0x88)
            continue;

        Bitboards::bit_set(result, ((new_sq >> 1) & 56) + (new_sq & 7));
    }

    return result;
}

u64 Bitboards::attacks_long(int square, u64 pieces, const std::vector<int> &pieces_moves)
{
    u64 result = 0ull;
    int sq = ((square & 56) << 1) + (square & 7);

    for(auto &move: pieces_moves)
    {
        int new_sq = sq;
        while (true)
        {
            new_sq += move;
            if (new_sq & 0x88)
                break;

            int bit = ((new_sq >> 1) & 56) + (new_sq & 7);

            Bitboards::bit_set(result, bit);

            if (Bitboards::bit_test(pieces, bit))
                break;
        }
    }

    return result;
}

u64 Bitboards::init_cols_forward(int square, int move)
{
    u64 result = 0ull;

    square += move;
    while (square >= 0 && square <= 63)
    {
        Bitboards::bit_set(result, square);
        square += move;
    }

    return result;
}

u64 Bitboards::init_isolated(int col)
{
    u64 result = 0ull;
    if (col > 0)
        result |= Bitboards::_cols[col-1];
    if (col < 7)
        result |= Bitboards::_cols[col+1];
    return result;
}

u64 Bitboards::init_passed(int color, int square)
{
    u64 result = this->_cols_forward[color][square];

    if (Bitboards::_col[square] > 0)
        result |= this->_cols_forward[color][square-1];
    if (Bitboards::_col[square] < 7)
        result |= this->_cols_forward[color][square+1];

    return result;
}

void Bitboards::init_magic(int square, Magic &magic, u64 magic_bb, const std::vector<int> &pieces_moves)
{
    u64 edges = ((Bitboards::_cols[0] | Bitboards::_cols[7]) & ~Bitboards::_cols[Bitboards::_col[square]]) |
            ((Bitboards::_rows[0] | Bitboards::_rows[7]) & ~Bitboards::_rows[Bitboards::_row[square]]);

    magic._magic = magic_bb;
    magic._mask = this->attacks_long(square, 0ull, pieces_moves) & ~edges;
    magic._shift = 64 - Bitboards::bits_count(magic._mask);

    u64 pieces = 0ull;
    do
    {
        magic._attacks[magic.index(pieces)] = this->attacks_long(square, pieces, pieces_moves);
        pieces = (pieces - magic._mask) & magic._mask;
    }
    while (pieces != 0ull);
}
