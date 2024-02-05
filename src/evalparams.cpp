#include "evalparams.h"

#include "board.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

static const std::string TYPES_NAMES[EvalParams::TYPES_COUNT] = {
    "PIECES_VALUES",
    "PST_KING",
    "PST_QUEEN",
    "PST_ROOK",
    "PST_BISHOP",
    "PST_KNIGHT",
    "PST_PAWN",
    "KING_SHIELD",
    "ATTACKER_WEIGHT",
    "ATTACKER_VALUE",
    "ATTACKER_SCALE",
    "KING_DEFENDERS",
    "PAWN_ISOLATED",
    "PAWN_DOUBLE",
    "PAWN_BACKWARD",
    "PAWN_BACKWARD_BLOCKED",
    "PAWN_PASSED",
    "PAWN_PASSED_KING",
    "PAWN_PASSED_KING_ENEMY",
    "KNIGHT_MOBILITY",
    "BISHOP_MOBILITY",
    "ROOK_MOBILITY",
    "QUEEN_MOBILITY",
    "TWO_BISHOPS",
    "KNIGHT_BEHIND_PAWN",
    "BISHOP_BEHIND_PAWN",
    "ROOK_ON7",
    "ROOK_ON_OPEN",
    "ENDGAME_SCALE",
    "GAME_PHASE",
    "NEURAL_SCALE"
};

/*
static const int TYPES_COUNTS[EvalParams::TYPES_COUNT][EvalParams::PHASES_COUNT][2] = {   // { start, count }
    { { 2, 0 }, { 2, 0 } },     //    PIECES_VALUES
    { { 40, 0 }, { 0, 0 } },    //    PST_KING
    { { 0, 0 }, { 0, 0 } },     //    PST_QUEEN
    { { 0, 0 }, { 0, 0 } },     //    PST_ROOK
    { { 0, 0 }, { 0, 0 } },     //    PST_BISHOP
    { { 0, 0 }, { 0, 0 } },     //    PST_KNIGHT
    { { 8, 0 }, { 8, 0 } },     //    PST_PAWN
    { { 0, 0 }, { 0, 0 } },     //    KING_SHIELD
    { { 2, 0 }, { 2, 0 } },     //    ATTACKER_WEIGHT
    { { 1, 0 }, { 1, 0 } },     //    ATTACKER_VALUE
    { { 0, 0 }, { 0, 0 } },     //    ATTACKER_SCALE
    { { 0, 0 }, { 0, 0 } },     //    KING_DEFENDERS
    { { 0, 0 }, { 0, 0 } },     //    PAWN_ISOLATED
    { { 0, 0 }, { 0, 0 } },     //    PAWN_DOUBLE
    { { 1, 0 }, { 1, 0 } },     //    PAWN_BACKWARD
    { { 1, 0 }, { 1, 0 } },     //    PAWN_BACKWARD_BLOCKED
    { { 1, 0 }, { 1, 0 } },     //    PAWN_PASSED
    { { 1, 0 }, { 1, 0 } },     //    PAWN_PASSED_KING
    { { 1, 0 }, { 1, 0 } },     //    PAWN_PASSED_KING_ENEMY
    { { 0, 0 }, { 0, 0 } },     //    KNIGHT_MOBILITY
    { { 0, 0 }, { 0, 0 } },     //    BISHOP_MOBILITY
    { { 0, 0 }, { 0, 0 } },     //    ROOK_MOBILITY
    { { 0, 0 }, { 0, 0 } },     //    QUEEN_MOBILITY
    { { 0, 0 }, { 0, 0 } },     //    TWO_BISHOPS
    { { 0, 0 }, { 0, 0 } },     //    KNIGHT_BEHIND_PAWN
    { { 0, 0 }, { 0, 0 } },     //    BISHOP_BEHIND_PAWN
    { { 0, 0 }, { 0, 0 } },     //    ROOK_ON7
    { { 0, 0 }, { 0, 0 } },     //    ROOK_ON_OPEN
    { { 0, 0 }, { 4, 0 } },     //    ENDGAME_SCALE
    { { 2, 0 }, { 0, 0 } },     //    GAME_PHASE
    { { 0, 0 }, { 0, 0 } }      //    NEURAL_SCALE
};
*/

/*
static const int TYPES_COUNTS[EvalParams::TYPES_COUNT][EvalParams::PHASES_COUNT][2] = {   // { start, count }
    { { 2, 5 }, { 2, 5 } },     //    PIECES_VALUES
    { { 40, 24 }, { 0, 64 } },  //    PST_KING
    { { 0, 64 }, { 0, 64 } },   //    PST_QUEEN
    { { 0, 64 }, { 0, 64 } },   //    PST_ROOK
    { { 0, 64 }, { 0, 64 } },   //    PST_BISHOP
    { { 0, 64 }, { 0, 64 } },   //    PST_KNIGHT
    { { 8, 48 }, { 8, 48 } },   //    PST_PAWN
    { { 0, 56 }, { 0, 0 } },    //    KING_SHIELD
    { { 2, 4 }, { 2, 4 } },     //    ATTACKER_WEIGHT
    { { 1, 4 }, { 1, 4 } },     //    ATTACKER_VALUE
    { { 0, 0 }, { 0, 0 } },     //    ATTACKER_SCALE
    { { 0, 9 }, { 0, 9 } },     //    KING_DEFENDERS
    { { 0, 8 }, { 0, 8 } },     //    PAWN_ISOLATED
    { { 0, 8 }, { 0, 8 } },     //    PAWN_DOUBLE
    { { 1, 4 }, { 1, 4 } },     //    PAWN_BACKWARD
    { { 1, 4 }, { 1, 4 } },     //    PAWN_BACKWARD_BLOCKED
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED_KING
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED_KING_ENEMY
    { { 0, 9 }, { 0, 9 } },     //    KNIGHT_MOBILITY
    { { 0, 14 }, { 0, 14 } },   //    BISHOP_MOBILITY
    { { 0, 15 }, { 0, 15 } },   //    ROOK_MOBILITY
    { { 0, 28 }, { 0, 28 } },   //    QUEEN_MOBILITY
    { { 0, 1 }, { 0, 1 } },     //    TWO_BISHOPS
    { { 0, 1 }, { 0, 1 } },     //    KNIGHT_BEHIND_PAWN
    { { 0, 1 }, { 0, 1 } },     //    BISHOP_BEHIND_PAWN
    { { 0, 1 }, { 0, 1 } },     //    ROOK_ON7
    { { 0, 2 }, { 0, 2 } },     //    ROOK_ON_OPEN
    { { 0, 0 }, { 4, 8 } },     //    ENDGAME_SCALE
    { { 2, 4 }, { 0, 0 } },     //    GAME_PHASE
    { { 0, 0 }, { 0, 0 } }      //    NEURAL_SCALE
};
*/

static const int TYPES_COUNTS[EvalParams::TYPES_COUNT][EvalParams::PHASES_COUNT][2] = {   // { start, count }
    { { 2, 5 }, { 2, 5 } },     //    PIECES_VALUES
    { { 40, 24 }, { 0, 64 } },  //    PST_KING
    { { 0, 64 }, { 0, 64 } },   //    PST_QUEEN
    { { 0, 64 }, { 0, 64 } },   //    PST_ROOK
    { { 0, 64 }, { 0, 64 } },   //    PST_BISHOP
    { { 0, 64 }, { 0, 64 } },   //    PST_KNIGHT
    { { 8, 48 }, { 8, 48 } },   //    PST_PAWN
    { { 0, 56 }, { 0, 0 } },    //    KING_SHIELD
    { { 2, 4 }, { 2, 4 } },     //    ATTACKER_WEIGHT
    { { 1, 4 }, { 1, 4 } },     //    ATTACKER_VALUE
    { { 0, 0 }, { 0, 0 } },     //    ATTACKER_SCALE
    { { 0, 9 }, { 0, 9 } },     //    KING_DEFENDERS
    { { 0, 8 }, { 0, 8 } },     //    PAWN_ISOLATED
    { { 0, 8 }, { 0, 8 } },     //    PAWN_DOUBLE
    { { 1, 4 }, { 1, 4 } },     //    PAWN_BACKWARD
    { { 1, 4 }, { 1, 4 } },     //    PAWN_BACKWARD_BLOCKED
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED_KING
    { { 1, 6 }, { 1, 6 } },     //    PAWN_PASSED_KING_ENEMY
    { { 0, 9 }, { 0, 9 } },     //    KNIGHT_MOBILITY
    { { 0, 14 }, { 0, 14 } },   //    BISHOP_MOBILITY
    { { 0, 15 }, { 0, 15 } },   //    ROOK_MOBILITY
    { { 0, 28 }, { 0, 28 } },   //    QUEEN_MOBILITY
    { { 0, 1 }, { 0, 1 } },     //    TWO_BISHOPS
    { { 0, 1 }, { 0, 1 } },     //    KNIGHT_BEHIND_PAWN
    { { 0, 1 }, { 0, 1 } },     //    BISHOP_BEHIND_PAWN
    { { 0, 1 }, { 0, 1 } },     //    ROOK_ON7
    { { 0, 2 }, { 0, 2 } },     //    ROOK_ON_OPEN
    { { 0, 0 }, { 4, 8 } },     //    ENDGAME_SCALE
    { { 2, 4 }, { 0, 0 } },     //    GAME_PHASE
    { { 0, 0 }, { 0, 0 } }      //    NEURAL_SCALE
};

static const std::vector<int> &PIECES_VALUES_MG = { 0, 0, 1311, 589, 377, 402, 100 };
static const std::vector<int> &PIECES_VALUES_EG = { 0, 0, 966, 589, 351, 338, 119 };

static const std::vector<int> &PST_KING_MG = {
    -50,    -50,    -50,    -50,    -50,    -50,    -50,    -50,
    -50,    -50,    -50,    -50,    -50,    -50,    -50,    -50,
    -50,    -50,    -50,    -50,    -50,    -50,    -50,    -50,
    -50,    -50,    -50,    -50,    -50,    -50,    -50,    -50,
    -50,    -50,    -50,    -50,    -50,    -50,    -53,    -72,
    -62,    -37,    -53,    -51,    -46,    -48,    -60,    -54,
      2,    -32,    -49,    -36,    -50,    -39,    -13,     13,
    -33,    -14,    -29,    -53,    -19,    -37,     23,     24
};

static const std::vector<int> &PST_KING_EG = {
    -43,     54,     43,     17,     26,     30,     21,    -52,
     23,     62,     57,     54,     37,     50,     43,     21,
     43,     65,     59,     47,     43,     49,     50,     34,
     26,     49,     50,     42,     36,     41,     37,     23,
      5,     22,     30,     29,     26,     25,     15,      9,
      2,      5,      9,     14,     14,     12,      5,     -7,
    -19,     -4,      1,     -5,      2,      2,    -13,    -34,
    -38,    -24,    -15,    -16,    -27,    -16,    -50,    -73
};

static const std::vector<int> &PST_QUEEN_MG = {
    79,     61,     69,     85,     98,     95,    113,    180,
    -4,    -19,     14,     29,     49,     53,     62,     98,
    25,     23,     11,     28,     55,    131,     98,     97,
    15,     12,      7,     15,     35,     31,     83,     58,
     7,    -13,      3,     24,     13,     23,     38,     44,
    19,     27,     18,     12,     19,     23,     15,     57,
    19,     19,     23,     22,     27,     31,     32,     57,
    -3,     13,     -3,      9,      0,    -16,    -19,     -8
};

static const std::vector<int> &PST_QUEEN_EG = {
    -27,      9,     23,     20,     13,      2,    -32,   -106,
     51,     90,     90,     91,     74,     65,     21,    -15,
      8,     43,     92,     88,     68,      9,     27,     -7,
     26,     58,     88,    100,     83,     84,     19,     14,
     21,     64,     62,     62,     65,     44,     22,     35,
    -39,    -18,     12,     18,     12,     18,     34,    -54,
    -37,    -24,    -21,    -24,    -39,    -49,    -84,    -67,
    -31,    -29,    -31,    -35,    -38,    -48,    -49,    -44
};

static const std::vector<int> &PST_ROOK_MG = {
    34,     45,     42,     37,     58,     74,     81,     96,
    10,     10,     39,     59,     69,     81,     78,     64,
     5,      1,     36,     63,     75,     81,     77,     15,
   -19,      2,     31,     38,     39,     44,     44,     14,
   -39,    -25,    -19,      2,     -3,     -6,     -1,    -30,
   -59,    -28,    -23,    -29,    -19,     -7,      9,    -33,
   -51,    -25,    -27,    -16,     -4,     -2,     -9,    -35,
   -25,    -16,    -20,    -10,     -8,     -6,     -9,    -24
};

static const std::vector<int> &PST_ROOK_EG = {
    27,     25,     23,     26,     24,     23,     20,      9,
    26,     25,     21,     13,      8,     11,      9,     14,
    34,     33,     24,     17,      7,     13,     14,     32,
    34,     30,     20,     11,     11,     16,     17,     19,
    23,     19,     22,      9,      9,     16,     17,     14,
    15,     -5,      0,     -5,    -12,     -9,     -8,     -1,
   -10,    -19,    -18,    -23,    -36,    -30,    -19,    -14,
   -11,    -24,    -17,    -19,    -35,    -25,    -13,     -8
};

static const std::vector<int> &PST_BISHOP_MG = {
    -4,    -52,    -48,    -14,    -15,    -16,     29,      8,
   -29,    -11,     38,      4,     13,     18,    -12,      9,
     5,     29,     23,     22,     57,     82,     57,     25,
     0,     29,     34,     57,     28,     49,     33,     -8,
    31,     10,     29,     33,     39,     12,     14,     58,
     9,     35,     19,     23,     21,     28,     21,     27,
    36,     12,     30,      8,     15,     16,     -9,    -11,
     8,     34,     19,     13,     21,    -13,    -14,    -40
};

static const std::vector<int> &PST_BISHOP_EG = {
    13,     31,     27,     19,     21,     17,     -7,     13,
    12,     19,     10,     20,     12,     13,     29,      8,
     5,      2,     19,     11,      4,     -1,      0,      6,
    -5,      0,     12,     18,     25,      1,      0,      9,
    -9,     -3,      7,     25,     17,     12,      3,    -20,
    -3,     -5,      1,     12,     17,      2,     -1,      4,
     2,     -9,     -6,      3,     -9,     -4,    -15,    -17,
    -1,    -10,     -7,     -4,    -13,    -24,    -10,     23
};

static const std::vector<int> &PST_KNIGHT_MG = {
    -189,    -81,    -73,    -14,     37,     73,    -37,    -25,
     -49,    -17,     15,     52,     72,     39,     41,     -9,
     -35,     21,     39,     40,     83,     83,     34,     37,
       9,      4,     20,     73,     21,     42,     23,     45,
      -5,      7,     23,     19,     31,     36,     45,     53,
     -30,    -20,    -12,     16,     21,     12,     15,    -14,
     -43,    -19,    -30,     -3,     -3,    -21,    -54,    -27,
      20,    -11,    -50,     -7,      7,    -45,    -42,    -59
};

static const std::vector<int> &PST_KNIGHT_EG = {
    -34,     15,     17,     17,     -4,      4,     13,     -5,
      1,     11,     13,    -12,      3,     -8,     -4,     -1,
      6,      7,     30,     38,     21,      4,      5,      5,
     -4,      5,     35,     24,     34,     28,     -6,     -9,
      2,      3,     29,     28,     24,     22,     -8,    -16,
    -34,      3,     12,     12,     14,     -4,    -16,    -14,
    -30,    -15,    -16,      0,    -10,    -24,    -12,    -29,
    -79,    -26,    -18,    -27,    -27,    -31,    -17,    -21
};

static const std::vector<int> &PST_PAWN_MG = {
    0,      0,      0,      0,      0,      0,      0,      0,
   98,     92,    143,    131,    123,     94,     99,    -16,
    4,     27,     18,     32,     51,     56,     61,     -9,
   -4,      1,     -7,     -2,     15,      8,     34,    -14,
   -7,     -7,    -13,    -14,      6,    -10,     18,    -21,
  -14,     -6,    -15,    -13,     -1,      7,     26,    -19,
  -15,    -16,    -43,    -55,    -31,     16,     18,    -28,
    0,      0,      0,      0,      0,      0,      0,      0
};

static const std::vector<int> &PST_PAWN_EG = {
    0,      0,      0,      0,      0,      0,      0,      0,
    5,     62,     38,     49,     52,     36,     68,     23,
    5,     22,      1,     -1,     -1,     -6,     29,     -3,
    3,     12,    -15,    -15,     -7,    -18,      1,    -12,
   -6,      7,     -9,    -15,    -10,    -12,      0,    -21,
  -13,      4,    -16,    -17,     -2,    -15,    -16,    -21,
   -5,     11,    -13,      5,      1,    -16,     -6,    -18,
    0,      0,      0,      0,      0,      0,      0,      0
};

static const std::vector<int> &KING_SHIELD_MG = { -6, 63, 60, 20, 18, -5, -16, 32, 1, -6, -36, -9, 28, -16, 33, -30, -10, 27, 30, 105, -21, 12, -4, -4, -35, -20, -4, -21, -26, -12, -25, -31, -61, -67, -26, -14, 4, 12, 13, -43, 20, -9, -5, 2, -21, -53, -16, -101, -9, -24, 32, 18, 31, 36, 60, 0 };
static const std::vector<int> &KING_SHIELD_EG = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const std::vector<int> &ATTACKER_WEIGHT_MG = { 0, 0, 21, 31, 32, 43, 0 };
static const std::vector<int> &ATTACKER_WEIGHT_EG = { 0, 0, -33, 17, 35, 30, 0 };
static const std::vector<int> &ATTACKER_VALUE_MG = { 0, -5, -7, -13, -19 };
static const std::vector<int> &ATTACKER_VALUE_EG = { 0, 2, 9, 6, 12 };
static const int ATTACKER_SCALE_MG = 10;
static const int ATTACKER_SCALE_EG = 20;
static const std::vector<int> &KING_DEFENDERS_MG = { -86, -17, 27, 61, 84, 45, 51, -50, 3 };
static const std::vector<int> &KING_DEFENDERS_EG = { 18, 4, 0, -11, -15, 6, -163, -30, -9 };

static const std::vector<int> &PAWN_ISOLATED_MG = { 6, -10, -29, -22, -24, -13, -24, 7 };
static const std::vector<int> &PAWN_ISOLATED_EG = { -7, -24, -3, -21, -28, -9, -15, -4 };

static const std::vector<int> &PAWN_DOUBLE_MG = { -34, -5, 11, -10, -12, 5, -10, -9 };
static const std::vector<int> &PAWN_DOUBLE_EG = { -25, -21, -24, -11, -8, -9, -15, -25 };

static const std::vector<int> &PAWN_BACKWARD_MG = { 0, 4, -7, 11, 18, 0, 0, 0 };
static const std::vector<int> &PAWN_BACKWARD_EG = { 0, 0, -4, -6, 14, 0, 0, 0 };
static const std::vector<int> &PAWN_BACKWARD_BLOCKED_MG = { 0, -18, -20, -2, 56, 0, 0, 0 };
static const std::vector<int> &PAWN_BACKWARD_BLOCKED_EG = { 0, -18, -21, -6, 23, 0, 0, 0 };

static const std::vector<int> &PAWN_PASSED_MG = { 0, 7, 7, 3, 64, 122, 172, 0 };
static const std::vector<int> &PAWN_PASSED_EG = { 0, 10, 10, 21, 38, 61, 91, 0 };

static const std::vector<int> &PAWN_PASSED_KING_MG = { 0, -3, -1, 2, -4, -12, -20, 0 };
static const std::vector<int> &PAWN_PASSED_KING_EG = { 0, 2, -3, -8, -14, -15, -20, 0 };

static const std::vector<int> &PAWN_PASSED_KING_ENEMY_MG = { 0, -1, -5, -3, -3, 3, 7, 0 };
static const std::vector<int> &PAWN_PASSED_KING_ENEMY_EG = { 0, 0, 4, 13, 25, 35, 42, 0 };

static const std::vector<int> &KNIGHT_MOBILITY_MG = { -90, -92, -41, -13, 7, 13, 24, 33, 44 };
static const std::vector<int> &KNIGHT_MOBILITY_EG = { -54, -21, -5, 6, 6, 17, 9, 4, -19 };

static const std::vector<int> &BISHOP_MOBILITY_MG = { -81, -43, -13, 1, 19, 32, 38, 42, 43, 49, 63, 71, 82, 52 };
static const std::vector<int> &BISHOP_MOBILITY_EG = { -113, -70, -35, -14, -1, 9, 17, 21, 27, 27, 25, 16, 18, -3 };

static const std::vector<int> &ROOK_MOBILITY_MG = { -212, -91, -35, -17, -8, -3, 1, 4, 12, 17, 23, 29, 32, 48, 67 };
static const std::vector<int> &ROOK_MOBILITY_EG = { -122, -78, -50, -14, -5, 4, 8, 11, 18, 22, 26, 26, 25, 17, -1 };

static const std::vector<int> &QUEEN_MOBILITY_MG = { -148, -106, -35, -89, -12, -10, 5, 8, 14, 23, 31, 34, 38, 43, 48, 49, 53, 54, 57, 66, 72, 93, 105, 155, 165, 189, 144, 179 };
static const std::vector<int> &QUEEN_MOBILITY_EG = { -132, -259, -119, 30, -74, -9, -3, 17, 24, 22, 23, 27, 37, 37, 33, 37, 36, 35, 33, 15, 3, -34, -47, -118, -127, -171, -113, -173 };

static const int TWO_BISHOPS_MG = 40;
static const int TWO_BISHOPS_EG = 79;

static const int KNIGHT_BEHIND_PAWN_MG = 6;
static const int KNIGHT_BEHIND_PAWN_EG = 27;
static const int BISHOP_BEHIND_PAWN_MG = 10;
static const int BISHOP_BEHIND_PAWN_EG = 25;

static const int ROOK_ON7_MG = -16;
static const int ROOK_ON7_EG = 27;

static const std::vector<int> &ROOK_ON_OPEN_MG = { 32, 9 };
static const std::vector<int> &ROOK_ON_OPEN_EG = { -2, 14 };

static const std::vector<int> &ENDGAME_SCALE_MG = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const std::vector<int> &ENDGAME_SCALE_EG = { -125, 5, 136, -19, -7, 23, 65, 137, 123, 83, 194, 136 };

static const std::vector<int> &GAME_PHASE_MG = { 0, 0, 122, 37, 18, 18, 0 };
static const std::vector<int> &GAME_PHASE_EG = { 0, 0, 0, 0, 0, 0, 0 };

static const int NEURAL_SCALE_MG = 0;
static const int NEURAL_SCALE_EG = 0;

EvalParams::EvalParams()
{
    this->start();

    this->_values[PIECES_VALUES][MG] = PIECES_VALUES_MG;
    this->_values[PIECES_VALUES][EG] = PIECES_VALUES_EG;

    this->_values[PST_KING][MG] = PST_KING_MG;
    this->_values[PST_KING][EG] = PST_KING_EG;

    this->_values[PST_QUEEN][MG] = PST_QUEEN_MG;
    this->_values[PST_QUEEN][EG] = PST_QUEEN_EG;

    this->_values[PST_ROOK][MG] = PST_ROOK_MG;
    this->_values[PST_ROOK][EG] = PST_ROOK_EG;

    this->_values[PST_BISHOP][MG] = PST_BISHOP_MG;
    this->_values[PST_BISHOP][EG] = PST_BISHOP_EG;

    this->_values[PST_KNIGHT][MG] = PST_KNIGHT_MG;
    this->_values[PST_KNIGHT][EG] = PST_KNIGHT_EG;

    this->_values[PST_PAWN][MG] = PST_PAWN_MG;
    this->_values[PST_PAWN][EG] = PST_PAWN_EG;

    this->_values[KING_SHIELD][MG] = KING_SHIELD_MG;
    this->_values[KING_SHIELD][EG] = KING_SHIELD_EG;

    this->_values[ATTACKER_WEIGHT][MG] = ATTACKER_WEIGHT_MG;
    this->_values[ATTACKER_WEIGHT][EG] = ATTACKER_WEIGHT_EG;

    this->_values[ATTACKER_VALUE][MG] = ATTACKER_VALUE_MG;
    this->_values[ATTACKER_VALUE][EG] = ATTACKER_VALUE_EG;

    this->_values[ATTACKER_SCALE][MG] = { ATTACKER_SCALE_MG };
    this->_values[ATTACKER_SCALE][EG] = { ATTACKER_SCALE_EG };

    this->_values[KING_DEFENDERS][MG] = KING_DEFENDERS_MG;
    this->_values[KING_DEFENDERS][EG] = KING_DEFENDERS_EG;

    this->_values[PAWN_ISOLATED][MG] = PAWN_ISOLATED_MG;
    this->_values[PAWN_ISOLATED][EG] = PAWN_ISOLATED_EG;

    this->_values[PAWN_DOUBLE][MG] = PAWN_DOUBLE_MG;
    this->_values[PAWN_DOUBLE][EG] = PAWN_DOUBLE_EG;

    this->_values[PAWN_BACKWARD][MG] = PAWN_BACKWARD_MG;
    this->_values[PAWN_BACKWARD][EG] = PAWN_BACKWARD_EG;

    this->_values[PAWN_BACKWARD_BLOCKED][MG] = PAWN_BACKWARD_BLOCKED_MG;
    this->_values[PAWN_BACKWARD_BLOCKED][EG] = PAWN_BACKWARD_BLOCKED_EG;

    this->_values[PAWN_PASSED][MG] = PAWN_PASSED_MG;
    this->_values[PAWN_PASSED][EG] = PAWN_PASSED_EG;

    this->_values[PAWN_PASSED_KING][MG] = PAWN_PASSED_KING_MG;
    this->_values[PAWN_PASSED_KING][EG] = PAWN_PASSED_KING_EG;

    this->_values[PAWN_PASSED_KING_ENEMY][MG] = PAWN_PASSED_KING_ENEMY_MG;
    this->_values[PAWN_PASSED_KING_ENEMY][EG] = PAWN_PASSED_KING_ENEMY_EG;

    this->_values[KNIGHT_MOBILITY][MG] = KNIGHT_MOBILITY_MG;
    this->_values[KNIGHT_MOBILITY][EG] = KNIGHT_MOBILITY_EG;

    this->_values[BISHOP_MOBILITY][MG] = BISHOP_MOBILITY_MG;
    this->_values[BISHOP_MOBILITY][EG] = BISHOP_MOBILITY_EG;

    this->_values[ROOK_MOBILITY][MG] = ROOK_MOBILITY_MG;
    this->_values[ROOK_MOBILITY][EG] = ROOK_MOBILITY_EG;

    this->_values[QUEEN_MOBILITY][MG] = QUEEN_MOBILITY_MG;
    this->_values[QUEEN_MOBILITY][EG] = QUEEN_MOBILITY_EG;

    this->_values[TWO_BISHOPS][MG] = { TWO_BISHOPS_MG };
    this->_values[TWO_BISHOPS][EG] = { TWO_BISHOPS_EG };

    this->_values[KNIGHT_BEHIND_PAWN][MG] = { KNIGHT_BEHIND_PAWN_MG };
    this->_values[KNIGHT_BEHIND_PAWN][EG] = { KNIGHT_BEHIND_PAWN_EG };

    this->_values[BISHOP_BEHIND_PAWN][MG] = { BISHOP_BEHIND_PAWN_MG };
    this->_values[BISHOP_BEHIND_PAWN][EG] = { BISHOP_BEHIND_PAWN_EG };

    this->_values[ROOK_ON7][MG] = { ROOK_ON7_MG };
    this->_values[ROOK_ON7][EG] = { ROOK_ON7_EG };

    this->_values[ROOK_ON_OPEN][MG] = ROOK_ON_OPEN_MG;
    this->_values[ROOK_ON_OPEN][EG] = ROOK_ON_OPEN_EG;

    this->_values[ENDGAME_SCALE][MG] = ENDGAME_SCALE_MG;
    this->_values[ENDGAME_SCALE][EG] = ENDGAME_SCALE_EG;

    this->_values[GAME_PHASE][MG] = GAME_PHASE_MG;
    this->_values[GAME_PHASE][EG] = GAME_PHASE_EG;

    this->_values[NEURAL_SCALE][MG] = { NEURAL_SCALE_MG };
    this->_values[NEURAL_SCALE][EG] = { NEURAL_SCALE_EG };

    this->init_pst();
}

void EvalParams::init_pst()
{
    this->_values[GAME_PHASE][MG][0] = 0;
    this->_values[GAME_PHASE][MG][0] += 2 * this->_values[EvalParams::GAME_PHASE][EvalParams::MG][Board::QUEEN];
    this->_values[GAME_PHASE][MG][0] += 4 * this->_values[EvalParams::GAME_PHASE][EvalParams::MG][Board::ROOK];
    this->_values[GAME_PHASE][MG][0] += 4 * this->_values[EvalParams::GAME_PHASE][EvalParams::MG][Board::BISHOP];
    this->_values[GAME_PHASE][MG][0] += 4 * this->_values[EvalParams::GAME_PHASE][EvalParams::MG][Board::KNIGHT];
    this->_values[GAME_PHASE][MG][0] += 16 * this->_values[EvalParams::GAME_PHASE][EvalParams::MG][Board::PAWN];

    for (int i = 0; i < 64; ++i)
    {
        this->pst_black_mg[0][i] = 0;
        this->pst_black_eg[0][i] = 0;
        this->pst_black_mg[1][i] = this->_values[EvalParams::PST_KING][EvalParams::MG][i];
        this->pst_black_eg[1][i] = this->_values[EvalParams::PST_KING][EvalParams::EG][i];

        this->pst_black_mg[2][i] = this->_values[EvalParams::PST_QUEEN][EvalParams::MG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::QUEEN];
        this->pst_black_eg[2][i] = this->_values[EvalParams::PST_QUEEN][EvalParams::EG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::QUEEN];
        this->pst_black_mg[3][i] = this->_values[EvalParams::PST_ROOK][EvalParams::MG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::ROOK];
        this->pst_black_eg[3][i] = this->_values[EvalParams::PST_ROOK][EvalParams::EG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::ROOK];
        this->pst_black_mg[4][i] = this->_values[EvalParams::PST_BISHOP][EvalParams::MG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::BISHOP];
        this->pst_black_eg[4][i] = this->_values[EvalParams::PST_BISHOP][EvalParams::EG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::BISHOP];
        this->pst_black_mg[5][i] = this->_values[EvalParams::PST_KNIGHT][EvalParams::MG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::KNIGHT];
        this->pst_black_eg[5][i] = this->_values[EvalParams::PST_KNIGHT][EvalParams::EG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::KNIGHT];
        this->pst_black_mg[6][i] = this->_values[EvalParams::PST_PAWN][EvalParams::MG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::MG][Board::PAWN];
        this->pst_black_eg[6][i] = this->_values[EvalParams::PST_PAWN][EvalParams::EG][i] + this->_values[EvalParams::PIECES_VALUES][EvalParams::EG][Board::PAWN];

        for (int j = 0; j < 7; ++j)
        {
            this->pst_white_mg[j][i^56] = this->pst_black_mg[j][i];
            this->pst_white_eg[j][i^56] = this->pst_black_eg[j][i];
        }
    }
}

EvalParams &EvalParams::instance()
{
    static EvalParams theSingleInstance;
    return theSingleInstance;
}

void EvalParams::save(std::string filename)
{
    std::ofstream file(filename);

    for (int i = 0; i < TYPES_COUNT; ++i)
    {
        file << "# " << TYPES_NAMES[i] << std::endl;

        file << this->_values[i][0].size() << std::endl;

        for (int j = 0; j < PHASES_COUNT; ++j)
        {
            for (auto val : this->_values[i][j])
                file << val << " ";
            file << std::endl;
        }

        file << std::endl;
    }

    file.close();
}

void EvalParams::save_fmt(std::string filename)
{
    std::ofstream file(filename);

    for (int i = 0; i < TYPES_COUNT; ++i)
    {
        file << "# " << TYPES_NAMES[i] << std::endl;

        int size = this->_values[i][0].size();
        for (int j = 0; j < PHASES_COUNT; ++j)
        {
            int item = 0;
            for (auto val : this->_values[i][j])
            {
                item++;

                if (size == 64)
                {
                    file << std::setw(6) << val << (item != size ? ", " : "");
                    if (item % 8 == 0)
                        file << std::endl;
                }
                else
                {
                    file << val << (item != size ? ", " : "");
                }
            }
            file << std::endl;
        }

        file << std::endl;
    }

    file.close();
}

void EvalParams::load(std::string filename)
{
    std::ifstream file(filename);

    std::string comment, line;
    int size;

    for (int i = 0; i < TYPES_COUNT; ++i)
    {
        std::getline(file, comment);
        std::getline(file, line);

        std::istringstream values(line);
        values >> size;

        for (int j = 0; j < PHASES_COUNT; ++j)
        {
            std::getline(file, line);
            std::istringstream values(line);

            this->_values[i][j].resize(size);
            for (int k = 0; k < size; k++)
                values >> this->_values[i][j][k];
        }

        std::getline(file, line);    // Пустая строка
    }

    file.close();

    this->init_pst();
}

void EvalParams::start()
{
    this->_type = 0;
    this->_phase = 0;
    this->_current = 0;
}

int *EvalParams::get_next()
{
    while (this->_current >= TYPES_COUNTS[this->_type][this->_phase][1])
    {
        this->_current = 0;
        this->_phase++;

        if (this->_phase >= EvalParams::PHASES_COUNT)
        {
            this->_phase = 0;
            this->_type++;
        }

        if (this->_type >= EvalParams::TYPES_COUNT)
        {
            this->start();
            std::cout << "end!" << std::endl;
            return nullptr;
        }
    }

    int cur_idx = TYPES_COUNTS[this->_type][this->_phase][0] + this->_current;

    std::cout << TYPES_NAMES[this->_type] << "[" << this->_phase << "][" << cur_idx << "] ";

    int *result = &this->_values[this->_type][this->_phase][cur_idx];
    this->_current++;
    return result;
}
