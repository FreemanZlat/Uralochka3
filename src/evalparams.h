#ifndef EVALPARAMS_H
#define EVALPARAMS_H

#include "common.h"

#include <vector>
#include <string>

class EvalParams
{
public:
    enum Type: u8
    {
        PIECES_VALUES = 0,

        PST_KING,
        PST_QUEEN,
        PST_ROOK,
        PST_BISHOP,
        PST_KNIGHT,
        PST_PAWN,

        KING_SHIELD,
        ATTACKER_WEIGHT,
        ATTACKER_VALUE,
        ATTACKER_SCALE,
        KING_DEFENDERS,

        PAWN_ISOLATED,
        PAWN_DOUBLE,
        PAWN_BACKWARD,
        PAWN_BACKWARD_BLOCKED,
        PAWN_PASSED,
        PAWN_PASSED_KING,
        PAWN_PASSED_KING_ENEMY,

        KNIGHT_MOBILITY,
        BISHOP_MOBILITY,
        ROOK_MOBILITY,
        QUEEN_MOBILITY,

        TWO_BISHOPS,

        KNIGHT_BEHIND_PAWN,
        BISHOP_BEHIND_PAWN,

        ROOK_ON7,
        ROOK_ON_OPEN,

        ENDGAME_SCALE,

        GAME_PHASE,

        NEURAL_SCALE,

        TYPES_COUNT
    };

    enum Phase: u8
    {
        MG = 0,
        EG,

        PHASES_COUNT
    };

    enum EndgameScale: u8
    {
        COMPLEXITY_DEFAULT = 0,
        COMPLEXITY_PAWNS,
        COMPLEXITY_PAWNS_FLANKS,
        COMPLEXITY_PAWNS_EG,
        SCALE_DRAW,
        SCALE_OCB_BISHOPS,
        SCALE_OCB_BISHOPS_PAWNS,
        SCALE_OCB_KNIGHT,
        SCALE_OCB_ROOK,
        SCALE_QUEEN,
        SCALE_LARGE_PAWN_ADV,
        SCALE_NORMAL
    };

    static EvalParams& instance();

    void save(std::string filename);
    void save_fmt(std::string filename);
    void load(std::string filename);

    void init_pst();

    void start();
    int *get_next();

    std::vector<int> _values[TYPES_COUNT][PHASES_COUNT];

    int pst_white_mg[7][64];
    int pst_white_eg[7][64];
    int pst_black_mg[7][64];
    int pst_black_eg[7][64];

private:
    EvalParams();
    EvalParams(const EvalParams& root) = delete;
    EvalParams& operator=(const EvalParams&) = delete;

    int _type;
    int _phase;
    int _current;
};

#endif // EVALPARAMS_H
