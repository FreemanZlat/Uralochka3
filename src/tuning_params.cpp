#include "tuning_params.h"

#include "moves.h"
#include "game.h"
#include "neural.h"

#include <cmath>

void Param::set(std::string n, bool t, double v, double mi, double ma, double c)
{
    this->name = n;
    this->need_tuning = t;
    this->value = v;
    this->min = mi;
    this->max = ma;
    this->c_end = (c < 0.0) ? (ma - mi) / 20.0 : c;
    this->r_end = 0.002;
}

TuningParams &TuningParams::instance()
{
    static TuningParams theSingleInstance;
    return theSingleInstance;
}

void TuningParams::set(Type type, double value)
{
    this->_params[type].value = value;

    switch (type)
    {
    case TUNE_SEE_PAWN:
        SEE_PICES_VALUES[Board::PAWN] = std::round(value);
        break;
    case TUNE_SEE_KNIGHT:
        SEE_PICES_VALUES[Board::KNIGHT] = std::round(value);
        break;
    case TUNE_SEE_BISHOP:
        SEE_PICES_VALUES[Board::BISHOP] = std::round(value);
        break;
    case TUNE_SEE_ROOK:
        SEE_PICES_VALUES[Board::ROOK] = std::round(value);
        break;
    case TUNE_SEE_QUEEN:
        SEE_PICES_VALUES[Board::QUEEN] = std::round(value);
        break;

    case TUNE_HIST_COUNT:
        HIST_COUNTER_COEFF = value;
        break;
    case TUNE_HIST_FOLLOW:
        HIST_FOLLOWER_COEFF = value;
        break;

    case TUNE_HIST_BON_A:
        HIST_BONUS_A = value;
        break;
    case TUNE_HIST_BON_B:
        HIST_BONUS_B = value;
        break;
    case TUNE_HIST_BON_C:
        HIST_BONUS_C = value;
        break;
    case TUNE_HIST_BON_MAX:
        HIST_BONUS_MAX = value;
        break;

    case TUNE_HIST_BON_N_A:
        HIST_BONUS_NEG_A = value;
        break;
    case TUNE_HIST_BON_N_B:
        HIST_BONUS_NEG_B = value;
        break;
    case TUNE_HIST_BON_N_C:
        HIST_BONUS_NEG_C = value;
        break;
    case TUNE_HIST_BON_N_MAX:
        HIST_BONUS_NEG_MAX = value;
        break;

    case TUNE_FUT_HIST_0:
        FUTILITY_PRUNING_HISTORY[0] = std::round(value);
        break;
    case TUNE_FUT_HIST_1:
        FUTILITY_PRUNING_HISTORY[1] = std::round(value);
        break;

    case TUNE_COUNT_DEPTH_0:
        COUNTER_PRUNING_DEPTH[0] = std::round(value);
        break;
    case TUNE_COUNT_DEPTH_1:
        COUNTER_PRUNING_DEPTH[1] = std::round(value);
        break;
    case TUNE_COUNT_HIST_0:
        COUNTER_PRUNING_HISTORY[0] = std::round(value);
        break;
    case TUNE_COUNT_HIST_1:
        COUNTER_PRUNING_HISTORY[1] = std::round(value);
        break;

    case TUNE_SEE_KILL:
        SEE_KILL = value;
        break;
    case TUNE_SEE_QUIET:
        SEE_QUIET = value;
        break;
    case TUNE_SEE_DEPTH:
        SEE_DEPTH = std::round(value);
        break;

    case TUNE_LMR_0_0:
        LMR_MOVES_0_0 = value;
        break;
    case TUNE_LMR_0_1:
        LMR_MOVES_0_1 = value;
        break;
    case TUNE_LMR_1_0:
        LMR_MOVES_1_0 = value;
        break;
    case TUNE_LMR_1_1:
        LMR_MOVES_1_1 = value;
        break;

    case TUNE_LMR_DEPTH_0:
        LMR_DEPTH_0 = value;
        break;
    case TUNE_LMR_DEPTH_1:
        LMR_DEPTH_1 = value;
        break;

    case TUNE_ASP_DELTA:
        ASPIRATION_DELTA = std::round(value);
        break;
    case TUNE_ASP_DELTA_INC:
        ASPIRATION_DELTA_INC = value;
        break;
    case TUNE_ASP_ALPHA:
        ASPIRATION_ALPHA_SHIFT = value;
        break;
    case TUNE_ASP_BETA:
        ASPIRATION_BETA_SHIFT = value;
        break;

    case TUNE_BETA_DEPTH:
        BETA_DEPTH = std::round(value);
        break;
    case TUNE_BETA_PRUN:
        BETA_PRUNING = value;
        break;
    case TUNE_BETA_IMPROV_0:
        BETA_IMPROV_0 = value;
        break;
    case TUNE_BETA_IMPROV_1:
        BETA_IMPROV_1 = value;
        break;
    case TUNE_BETA_HASHHIT_0:
        BETA_HASHHIT_0 = value;
        break;
    case TUNE_BETA_HASHHIT_1:
        BETA_HASHHIT_1 = value;
        break;
    case TUNE_BETA_RETURN:
        BETA_RETURN = value;
        break;

    // case TUNE_ALPHA_PRUN:
    //     ALPHA_PRUNING = std::round(value);
    //     break;

    case TUNE_NULL_MIN:
        NULL_MIN = value;
        break;
    case TUNE_NULL_REDUCTION:
        NULL_REDUCTION = value;
        break;
    case TUNE_NULL_DIV_1:
        NULL_DIV_1 = value;
        break;
    case TUNE_NULL_DIV_2:
        NULL_DIV_2 = value;
        break;

    case TUNE_PROBCUT_DEPTH:
        PROBCUT_DEPTH = std::round(value);
        break;
    case TUNE_PROBCUT_BETA:
        PROBCUT_BETA = std::round(value);
        break;

    case TUNE_IIR_PV_RED:
        IIR_PV_REDUCTION = std::round(value);
        break;
    case TUNE_IIR_CUT_DEPTH:
        IIR_CUT_DEPTH = std::round(value);
        break;
    case TUNE_IIR_CUT_RED:
        IIR_CUT_REDUCTION = std::round(value);
        break;

    case TUNE_FUT_MARGIN_0:
        FUT_MARGIN_0 = std::round(value);
        break;
    case TUNE_FUT_MARGIN_1:
        FUT_MARGIN_1 = std::round(value);
        break;
    case TUNE_FUT_MARGIN_2:
        FUT_MARGIN_2 = std::round(value);
        break;

    case TUNE_SING_DEPTH_1:
        SINGULAR_DEPTH_1 = std::round(value);
        break;
    case TUNE_SING_DEPTH_2:
        SINGULAR_DEPTH_2 = std::round(value);
        break;
    case TUNE_SING_COEFF:
        SINGULAR_COEFF = value;
        break;
    case TUNE_SING_EXTS:
        SINGULAR_EXTS = std::round(value);
        break;
    // case TUNE_SING_BETA:
    //     SINGULAR_BETA = std::round(value);
    //     break;

    case TUNE_HIST_REDUCT:
        HISTORY_REDUCTION = std::round(value);
        break;

    case TUNE_EVAL_DIVIDER:
        EVAL_DIVIDER = value;
        break;

    case TUNE_TIME_MID:
        TIME_MID = std::round(value);
        break;
    case TUNE_TIME_MID_VAL:
        TIME_MID_VAL = value;
        break;
    case TUNE_TIME_C_MIN:
        TIME_INC_COEF_MIN = value;
        break;
    case TUNE_TIME_D_MIN:
        TIME_INC_DIV_MIN = value;
        break;
    case TUNE_TIME_C_MAX:
        TIME_INC_COEF_MAX = value;
        break;
    case TUNE_TIME_D_MAX:
        TIME_INC_DIV_MAX = value;
        break;

    case TUNE_CH_COEFF:
        CORRHIST_COEFF = value;
        break;
    case TUNE_CH_PAWN:
        CORRHIST_PAWN = value;
        break;

    case TUNE_END:
        break;
    }
}

bool TuningParams::set(const std::string &name, double value)
{
    for (int type = 0; type < TuningParams::TUNE_END; type++)
        if (this->_params[type].name == name)
        {
            this->set((TuningParams::Type)type, value);
            return true;
        }
    return false;
}

TuningParams::TuningParams()
{
    this->_params.resize(TuningParams::TUNE_END);

    this->_params[TuningParams::TUNE_SEE_PAWN].set(     "TUNE_SEE_PAWN",        true,    98.6455,   85.0,       110.0   );
    this->_params[TuningParams::TUNE_SEE_KNIGHT].set(   "TUNE_SEE_KNIGHT",      true,   395.6142,   350.0,      450.0   );
    this->_params[TuningParams::TUNE_SEE_BISHOP].set(   "TUNE_SEE_BISHOP",      true,   410.4965,   350.0,      450.0   );
    this->_params[TuningParams::TUNE_SEE_ROOK].set(     "TUNE_SEE_ROOK",        true,   658.6571,   500.0,      700.0   );
    this->_params[TuningParams::TUNE_SEE_QUEEN].set(    "TUNE_SEE_QUEEN",       true,   1278.8269,  1150.0,     1400.0  );

    this->_params[TuningParams::TUNE_HIST_COUNT].set(   "TUNE_HIST_COUNT",      true,   0.9013,     0.0,        2.0 );
    this->_params[TuningParams::TUNE_HIST_FOLLOW].set(  "TUNE_HIST_FOLLOW",     true,   0.8799,     0.0,        2.0  );

    this->_params[TuningParams::TUNE_HIST_BON_A].set(   "TUNE_HIST_BON_A",      true,   1.3379,     0.0,        3.0  );
    this->_params[TuningParams::TUNE_HIST_BON_B].set(   "TUNE_HIST_BON_B",      true,   2.9276,     -8.0,       8.0  );
    this->_params[TuningParams::TUNE_HIST_BON_C].set(   "TUNE_HIST_BON_C",      true,   -1.0482,    -8.0,       8.0  );
    this->_params[TuningParams::TUNE_HIST_BON_MAX].set( "TUNE_HIST_BON_MAX",    true,   393.9920,   200.0,      800.0  );

    this->_params[TuningParams::TUNE_HIST_BON_N_A].set( "TUNE_HIST_BON_N_A",    true,   1.1305,     0.0,        3.0  );
    this->_params[TuningParams::TUNE_HIST_BON_N_B].set( "TUNE_HIST_BON_N_B",    true,   2.7296,     -8.0,       8.0  );
    this->_params[TuningParams::TUNE_HIST_BON_N_C].set( "TUNE_HIST_BON_N_C",    true,   3.2039,     -8.0,       8.0  );
    this->_params[TuningParams::TUNE_HIST_BON_N_MAX].set("TUNE_HIST_BON_N_MAX", true,   359.7403,   200.0,      800.0  );

    this->_params[TuningParams::TUNE_FUT_HIST_0].set(   "TUNE_FUT_HIST_0",      true,   11757.4104, 11000.0,    13000.0 );
    this->_params[TuningParams::TUNE_FUT_HIST_1].set(   "TUNE_FUT_HIST_1",      true,   5968.8154,  5400.0,     6600.0  );

    this->_params[TuningParams::TUNE_COUNT_DEPTH_0].set("TUNE_COUNT_DEPTH_0",   false,  3.0,        1.0,        5.0,    1.0     );
    this->_params[TuningParams::TUNE_COUNT_DEPTH_1].set("TUNE_COUNT_DEPTH_1",   false,  2.0,        1.0,        5.0,    1.0     );
    this->_params[TuningParams::TUNE_COUNT_HIST_0].set( "TUNE_COUNT_HIST_0",    true,   -1029.6176, -1150.0,    -900.0  );
    this->_params[TuningParams::TUNE_COUNT_HIST_1].set( "TUNE_COUNT_HIST_1",    true,   -2564.3647, -2800.0,    -2400.0 );

    this->_params[TuningParams::TUNE_SEE_KILL].set(     "TUNE_SEE_KILL",        true,   -17.3012,   -22.0,      -10.0   );
    this->_params[TuningParams::TUNE_SEE_QUIET].set(    "TUNE_SEE_QUIET",       true,   -66.5522,   -75.0,      -50.0   );
    this->_params[TuningParams::TUNE_SEE_DEPTH].set(    "TUNE_SEE_DEPTH",       true,   11.0701,    3.0,        15.0    );

    this->_params[TuningParams::TUNE_LMR_0_0].set(      "TUNE_LMR_0_0",         true,   1.8889,     1.8,        2.1     );
    this->_params[TuningParams::TUNE_LMR_0_1].set(      "TUNE_LMR_0_1",         true,   2.3941,     2.2,        2.8     );
    this->_params[TuningParams::TUNE_LMR_1_0].set(      "TUNE_LMR_1_0",         true,   3.9836,     3.5,        4.5     );
    this->_params[TuningParams::TUNE_LMR_1_1].set(      "TUNE_LMR_1_1",         true,   3.9156,     3.5,        4.5     );

    this->_params[TuningParams::TUNE_LMR_DEPTH_0].set(  "TUNE_LMR_DEPTH_0",     true,   0.2383,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_LMR_DEPTH_1].set(  "TUNE_LMR_DEPTH_1",     true,   2.4858,     2.0,        2.8     );

    this->_params[TuningParams::TUNE_ASP_DELTA].set(     "TUNE_ASP_DELTA",      true,   19.8097,    10,         30.0    );
    this->_params[TuningParams::TUNE_ASP_DELTA_INC].set( "TUNE_ASP_DELTA_INC",  true,   1.1731,     1.0,        4.0     );
    this->_params[TuningParams::TUNE_ASP_ALPHA].set(     "TUNE_ASP_ALPHA",      true,   0.8427,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_ASP_BETA].set(      "TUNE_ASP_BETA",       true,   0.4607,     0.0,        1.0     );

    this->_params[TuningParams::TUNE_BETA_DEPTH].set(    "TUNE_BETA_DEPTH",     true,   13.9219,    4.0,        20.0    );
    this->_params[TuningParams::TUNE_BETA_PRUN].set(     "TUNE_BETA_PRUN",      true,   78.8325,    60.0,       90.0    );
    this->_params[TuningParams::TUNE_BETA_IMPROV_0].set( "TUNE_BETA_IMPROV_0",  true,   -9.5820,    -30.0,      30.0    );
    this->_params[TuningParams::TUNE_BETA_IMPROV_1].set( "TUNE_BETA_IMPROV_1",  true,   80.3477,    60.0,       90.0    );
    this->_params[TuningParams::TUNE_BETA_HASHHIT_0].set("TUNE_BETA_HASHHIT_0", true,   8.3672,     -30,        30.0    );
    this->_params[TuningParams::TUNE_BETA_HASHHIT_1].set("TUNE_BETA_HASHHIT_1", true,   -5.7563,    -30,        30.0    );
    this->_params[TuningParams::TUNE_BETA_RETURN].set(   "TUNE_BETA_RETURN",    true,   0.7812,     0.0,        1.0     );

    // this->_params[TuningParams::TUNE_ALPHA_PRUN].set(    "TUNE_ALPHA_PRUN",     false,  3009.9027,  2600.0,     3400.0  );

    this->_params[TuningParams::TUNE_NULL_MIN].set(      "TUNE_NULL_MIN",       true,   3.5839,     0.0,        10.0,   1.0     );
    this->_params[TuningParams::TUNE_NULL_REDUCTION].set("TUNE_NULL_REDUCTION", true,   3.0861,     0.0,        10.0,   1.0     );
    this->_params[TuningParams::TUNE_NULL_DIV_1].set(    "TUNE_NULL_DIV_1",     true,   205.0134,   175.0,      225.0   );
    this->_params[TuningParams::TUNE_NULL_DIV_2].set(    "TUNE_NULL_DIV_2",     true,   2.0554,     2.0,        10.0,   1.0     );

    this->_params[TuningParams::TUNE_PROBCUT_DEPTH].set( "TUNE_PROBCUT_DEPTH",  true,   6.7374,     2.0,        10.0,   1.0     );
    this->_params[TuningParams::TUNE_PROBCUT_BETA].set(  "TUNE_PROBCUT_BETA",   true,   118.1262,   95.0,       135.0   );

    this->_params[TuningParams::TUNE_IIR_PV_RED].set(    "TUNE_IIR_PV_RED",     true,   4.5263,     0.0,        5.0,    1.0     );
    this->_params[TuningParams::TUNE_IIR_CUT_DEPTH].set( "TUNE_IIR_CUT_DEPTH",  true,   6.0245,     5.0,        12.0,   1.0     );
    this->_params[TuningParams::TUNE_IIR_CUT_RED].set(   "TUNE_IIR_CUT_RED",    true,   2.7407,     0.0,        5.0,    1.0     );

    this->_params[TuningParams::TUNE_FUT_MARGIN_0].set(  "TUNE_FUT_MARGIN_0",   true,   88.7158,    75.0,       110.0   );
    this->_params[TuningParams::TUNE_FUT_MARGIN_1].set(  "TUNE_FUT_MARGIN_1",   true,   54.5391,    45.0,       80.0    );
    this->_params[TuningParams::TUNE_FUT_MARGIN_2].set(  "TUNE_FUT_MARGIN_2",   true,   158.6382,   140.0,      180.0   );

    this->_params[TuningParams::TUNE_SING_DEPTH_1].set(  "TUNE_SING_DEPTH_1",   true,   6.1287,     2.0,        12.0,   1.0     );
    this->_params[TuningParams::TUNE_SING_DEPTH_2].set(  "TUNE_SING_DEPTH_2",   true,   2.7602,     0.0,        10.0,   1.0     );
    this->_params[TuningParams::TUNE_SING_COEFF].set(    "TUNE_SING_COEFF",     true,   0.4489,     0.1,        1.0     );
    this->_params[TuningParams::TUNE_SING_EXTS].set(     "TUNE_SING_EXTS",      true,   8.0860,     4.0,        20.0,   1.0     );
    // this->_params[TuningParams::TUNE_SING_BETA].set(     "TUNE_SING_BETA",      true,   15.0,       5.0,        30.0    );

    this->_params[TuningParams::TUNE_HIST_REDUCT].set(   "TUNE_HIST_REDUCT",    true,   5250.4383,  4400.0,     5600.0  );

    this->_params[TuningParams::TUNE_EVAL_DIVIDER].set(  "TUNE_EVAL_DIVIDER",   true,   497.8580,   400.0,      600.0   );

    this->_params[TuningParams::TUNE_TIME_MID].set(      "TUNE_TIME_MID",       true,   45.6674,    35,         60.0    );
    this->_params[TuningParams::TUNE_TIME_MID_VAL].set(  "TIME_MID_VAL",        true,   0.5146,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_TIME_C_MIN].set(    "TUNE_TIME_C_MIN",     false,  25.0,       10,         50.0    );
    this->_params[TuningParams::TUNE_TIME_D_MIN].set(    "TUNE_TIME_D_MIN",     false,  50.0,       25,         75.0    );
    this->_params[TuningParams::TUNE_TIME_C_MAX].set(    "TUNE_TIME_C_MAX",     false,  25.0,       10,         50.0    );
    this->_params[TuningParams::TUNE_TIME_D_MAX].set(    "TUNE_TIME_D_MAX",     false,  8.0,        2,          40.0    );

    this->_params[TuningParams::TUNE_CH_COEFF].set(      "TUNE_CH_COEFF",       true,   69.1454,    32.0,       128.0,  20.0    );
    this->_params[TuningParams::TUNE_CH_PAWN].set(       "TUNE_CH_PAWN",        false,  3200.0000,  2400.0,     4000.0    );

    for (int type = 0; type < TuningParams::TUNE_END; type++)
        this->set((TuningParams::Type)type, this->_params[type].value);
}


