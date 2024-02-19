#ifndef HASH_H
#define HASH_H

#include "common.h"

class Zorbist
{
public:
    static Zorbist& instance();

    u64 _white_move;
    u64 _white_00;
    u64 _white_000;
    u64 _black_00;
    u64 _black_000;
    u64 _en_passant[64];
    u64 _pieces[32][64];

private:
    Zorbist();
    Zorbist(const Zorbist &) = delete;
    Zorbist& operator=(const Zorbist &) = delete;
};

struct TTNode
{
    enum Type: u8
    {
        NONE = 0,
        ALPHA,
        EXACT,
        BETA
    };

    u16 _hash;
    u16 _move;
    i16 _value;
    i16 _eval;
    u8 _depth;
    u8 _age_type;
};

#define CELL_SIZE 3

struct TTCell
{
    TTNode nodes[CELL_SIZE];
    u16 padding;
};

class TranspositionTable
{
public:
    static TranspositionTable& instance();
    void init(int size);
    void destroy();
    void clear();
    void disable();
    void save(u64 hash, int depth, int value, int eval, TTNode::Type type, int move, u8 age);
    bool load(u64 hash, TTNode &node);
    int get_percent();
    void prefetch(u64 hash);

private:
    TranspositionTable();
    ~TranspositionTable();
    TranspositionTable(const TranspositionTable& root) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    TTCell *_table;
    u32 _table_size;
//    std::atomic<u32> _used_count;
    bool _enabled;
};

#endif // HASH_H
