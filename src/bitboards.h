#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "common.h"

#include <vector>

struct Magic
{
    u64 _magic;
    u64 _mask;
    int _shift;
    u64 _attacks[4096];
    Magic();
    int index(u64 pieces) const;
};

class Bitboards
{
public:
    static Bitboards& instance();

    static void print(const u64 &bitboard);
    static int lsb(const u64 bitboard);
    static int msb(const u64 bitboard);
    static int poplsb(u64 &bitboard);
    static int bits_count(const u64 bitboard);
    static void bit_set(u64 &bitboard, int bit);
    static void bit_clear(u64 &bitboard, int bit);
    static bool bit_test(const u64 bitboard, int bit);
    static bool several(const u64 bitboard);
    static u64 pawns_moves(const u64 pawns, const u64 all, int color);

    bool _dark[64];

    u64 _board_left;
    u64 _board_right;

    u64 _moves_king[64];
    u64 _moves_knight[64];
    u64 _attacks_rooks[64];
    u64 _attacks_bishops[64];
    u64 _attacks_pawn[2][64];

    u64 _cols_forward[2][64];

    u64 _isolated[8];
    u64 _passed[2][64];

    u64 _bits_between[64][64];

    Magic _magic_rooks[64];
    Magic _magic_bishops[64];

    static const u64 _cols[8];
    static const u64 _rows[8];

    static const u8 _col[64];
    static const u8 _row[64];

    static const u64 _darks;
    static const u64 _lights;

private:
    Bitboards();
    Bitboards(const Bitboards &) = delete;
    Bitboards& operator=(const Bitboards &) = delete;

    void init();
    u64 moves_short(int square, const std::vector<int> &pieces_moves);
    u64 attacks_long(int square, u64 pieces, const std::vector<int> &pieces_moves);
    u64 init_cols_forward(int square, int move);
    u64 init_isolated(int col);
    u64 init_passed(int color, int square);
    void init_magic(int square, Magic &magic, u64 magic_bb, const std::vector<int> &pieces_moves);
};

#endif // BITBOARDS_H
