#ifndef MOVES_H
#define MOVES_H

#include "common.h"
#include "bitboards.h"

#include <vector>
#include <string>

class Board;

// Структура хода
struct Move
{
    u16 _move;
    int _value;
    Move(u16 move, int value = 0);
    std::string get_string();
    static std::string get_string(u16 move);

    static const u16 KILLED = 32768;
};

struct History
{
    int _history[2][64][64];
    int _followers[2][7][64][7][64];
    u16 _counter_moves[2][7][64];
    History();
};

typedef Move* PMove;

class Moves
{
public:
    Moves();
    void init(Board *board, History *history);
    void clear_killers();
    void init_generator(bool kills, int ply, u16 move_counter, int piece_counter, u16 move_follower, int piece_follower);
    void update_hash(u16 move);
    void update_history(int depth, int color);
    void update_killers(u16 move);
    int get_history(int color, u16 move, int piece, int &counter_hist, int &follower_hist);
    u16 get_next(bool skip_quiets = false);
    bool is_killer(u16 move);
    int SEE(u16 move);

    enum stages
    {
        STAGE_HASH = 0,
        STAGE_GEN_KILLS,
        STAGE_GOOD_KILLS,
        STAGE_KILLER1,
        STAGE_KILLER2,
        STAGE_COUNTER,
        STAGE_GEN_QUIET,
        STAGE_QUIET,
        STAGE_BAD_KILLS,
        STAGE_END
    };

    std::vector<int> _hist_moves;

private:
    int see(int to, int color, int score, int target, u64 all);
    void clear();
    bool is_legal(u16 move);
    void add(int piece = 0, int from = 0, int to = 0, int killed = 0, int pawn_morph = 0, int en_passant = 255);
    void gen_kills();
    void gen_quiet();

    void kills_pawn(int square, int color, int en_passant, const u64 pieces_moves);
    void kills_short(int square, int color,const u64 pieces_moves);
    void kills_long(int square, int color, const Magic &magic);

    void moves_pawn(int square, int color);
    void moves_short(int square, int color, const u64 pieces_moves);
    void moves_long(int square, int color, const Magic &magic);
    void moves_king(int square, int color, int flags, const u64 pieces_moves);
    bool check_00_000(int square, int piece_move, int count_empty, bool is_white);

    void history_bonus(int &node, int bonus);

    u64 attacks_all(int pos, int color, u64 all);

    std::vector<Move> _moves_kills;
    std::vector<Move> _moves_quiet;
    Board *_board;
    History *_history;
    bool _generated;
    bool _kills;
    int _ply;
    u16 _hash;
    u16 _hash_move;
    u16 _killer1;
    u16 _killer2;
    u16 _counter;
    u16 _killer1_move;
    u16 _killer2_move;
    u16 _counter_move;
    stages _stage;
    u16 _move_counter;
    int _piece_counter;
    u16 _move_follower;
    int _piece_follower;
};

#endif // MOVES_H
