#include "hash.h"

#include <iostream>

#if defined(__linux__)
    #include <sys/mman.h>
#endif

Zorbist &Zorbist::instance()
{
    static Zorbist theSingleInstance;
    return theSingleInstance;
}

Zorbist::Zorbist()
{
    Randomizer rnd;

    this->_white_move = rnd.random();
    this->_white_00 = rnd.random();
    this->_white_000 = rnd.random();
    this->_black_00 = rnd.random();
    this->_black_000 = rnd.random();

    for (auto &item : this->_en_passant)
        item = rnd.random();

    for (int piece = 0; piece < 32; ++piece)
        for (auto &item : this->_pieces[piece])
            item = rnd.random();
}

TranspositionTable &TranspositionTable::instance()
{
    static TranspositionTable theSingleInstance;
    return theSingleInstance;
}

void TranspositionTable::init(int size)
{
    this->destroy();

    const u64 MB = 1ull << 20;

    u32 table_size = size * MB / sizeof(TTCell);

    std::cout << "info string Hash size: " << table_size <<
                 " -- MB: " << table_size * sizeof(TTCell) / MB << std::endl;

#if defined(__linux__) && !defined(__ANDROID__)
    this->_table = (TTCell *)aligned_alloc(2 * MB, table_size * sizeof(TTCell));
    madvise(this->_table, table_size * sizeof(TTCell), MADV_HUGEPAGE);
#else
    this->_table = (TTCell *)malloc(table_size * sizeof(TTCell));
#endif

    this->_table_size = table_size;
    this->clear();
    this->_enabled = true;
}

void TranspositionTable::destroy()
{
    if (this->_table_size > 0)
    {
        free(this->_table);
        this->_table_size = 0;
    }
}

void TranspositionTable::clear()
{
    if (!this->_enabled)
        return;

//    std::cout << "info string Clear Hash" << std::endl;

    for (u32 c = 0; c < this->_table_size; ++c)
    {
        auto &cell = this->_table[c];
        for (int i = 0; i < CELL_SIZE; ++i)
        {
            cell.nodes[i]._hash = 0;
            cell.nodes[i]._depth = 0;
            cell.nodes[i]._value = 0;
            cell.nodes[i]._eval = 0;
            cell.nodes[i]._move = 0;
            cell.nodes[i]._age_type = 0;
        }
    }
}

void TranspositionTable::disable()
{
    this->clear();
    this->_enabled = false;
}

void TranspositionTable::save(u64 hash, int depth, int value, int eval, TTNode::Type type, int move, u8 age)
{
    if (!this->_enabled)
        return;

    u16 hash16 = static_cast<u16>(hash >> 48);

    const u8 AGE_MASK = 0b11111100;

    age = age << 2;

    TTNode new_node;
    new_node._hash = hash16;
    new_node._depth = depth;
    new_node._value = value;
    new_node._eval = eval;
    new_node._move = move;
    new_node._age_type = age | type;

    u32 idx = hash % this->_table_size;
    auto &cell = this->_table[idx];

    auto *node = &cell.nodes[0];

    for (int i = 0; i < CELL_SIZE; ++i)
    {
        if (/*cell.nodes[i]._hash == 0 || */cell.nodes[i]._hash == hash16)
        {
            node = &cell.nodes[i];
            break;
        }

        if (((cell.nodes[i]._age_type & AGE_MASK) == age) - ((node->_age_type & AGE_MASK) == age) - (cell.nodes[i]._depth < node->_depth) < 0)
            node = &cell.nodes[i];
    }

    if (depth == 0 && node->_depth > 0 && (node->_age_type & AGE_MASK) == age)
        return;

//  HAHAHAHAHAHAHAAAA!!!
//    if (node->_hash == 0 && node->_depth == 0 && node->_value == 0 && node->_eval == 0 && node->_move == 0 && node->_age_type == 0)
//        this->_used_count++;

    *node = new_node;
}

bool TranspositionTable::load(u64 hash, TTNode &node)
{
    if (this->_enabled)
    {
        u16 hash16 = static_cast<u16>(hash >> 48);

        u32 idx = hash % this->_table_size;
        auto &cell = this->_table[idx];

        for (int i = 0; i < CELL_SIZE; ++i)
            if (cell.nodes[i]._hash == hash16)
            {
                node = cell.nodes[i];
                node._age_type = node._age_type & 3;
                return true;
            }
    }

    node._hash = 0;
    node._depth = 0;
    node._value = 0;
    node._eval = 0;
    node._move = 0;
    node._age_type = 0;
    return false;
}

int TranspositionTable::get_percent()
{
    return 0;
}

void TranspositionTable::prefetch(u64 hash)
{
    if (this->_enabled)
    {
        u32 idx = hash % this->_table_size;
        __builtin_prefetch(&this->_table[idx]);
//        _mm_prefetch(&this->_table[idx], _MM_HINT_T0);
    }
}

TranspositionTable::TranspositionTable()
{
    this->_enabled = false;
    this->_table_size = 0;
    this->init(32);
}

TranspositionTable::~TranspositionTable()
{
    this->destroy();
}
