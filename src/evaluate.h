#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"
#include "evalparams.h"

#include <vector>

class Evaluate
{
public:
    Evaluate();
    void init(Board *board);
    int eval(int ply, int alpha, int beta);
    int eval_classic();
    int eval_neural(int ply);
    int eval_flipped(int ply);

private:
    void eval_kings(EvalParams &params, int color, int &eval_mg, int &eval_eg);
    void eval_queens(EvalParams &params, int color, int &eval_mg, int &eval_eg);
    void eval_rooks(EvalParams &params, int color, int &eval_mg, int &eval_eg);
    void eval_bishops(EvalParams &params, int color, int &eval_mg, int &eval_eg);
    void eval_knights(EvalParams &params, int color, int &eval_mg, int &eval_eg);
    void eval_pawns(EvalParams &params, int color, int &eval_mg, int &eval_eg);

    int endgame_scale(EvalParams &params, int eval_eg);

    int _game_phase;
    u64 _all;
    u64 _all_minus_bishops[2];
    u64 _all_minus_rooks[2];
    u64 _mobility[2];
    u64 _attacks[2];
    u64 _pawn_attacks[2];
    u64 _king_area[2];
    int _king_attackers[2];
    int _king_attackers_weight_mg[2];
    int _king_attackers_weight_eg[2];

    Board *_board;
    int distance[64][64];

    int _count_q[2];
    int _count_r[2];
    int _count_bl[2];
    int _count_bd[2];
    int _count_n[2];
    int _count_p[2];
};

#endif // EVALUATE_H
