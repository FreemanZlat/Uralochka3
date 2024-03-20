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
        HIST_COUNTER_COEFF = std::round(value);
        break;
    case TUNE_HIST_FOLLOW:
        HIST_FOLLOWER_COEFF = std::round(value);
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
    case TUNE_ALPHA_PRUN:
        ALPHA_PRUNING = std::round(value);
        break;

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
    //  http://freemanzlat.ddns.net:7654/tune/126/
    //  2700 iters

    this->_params.resize(TuningParams::TUNE_END);

    this->_params[TuningParams::TUNE_SEE_PAWN].set(     "TUNE_SEE_PAWN",        true,   99.7321,    85.0,       110.0   );
    this->_params[TuningParams::TUNE_SEE_KNIGHT].set(   "TUNE_SEE_KNIGHT",      true,   394.2311,   350.0,      450.0   );
    this->_params[TuningParams::TUNE_SEE_BISHOP].set(   "TUNE_SEE_BISHOP",      true,   399.1987,   350.0,      450.0   );
    this->_params[TuningParams::TUNE_SEE_ROOK].set(     "TUNE_SEE_ROOK",        true,   653.6240,   500.0,      700.0   );
    this->_params[TuningParams::TUNE_SEE_QUEEN].set(    "TUNE_SEE_QUEEN",       true,   1280.8153,  1150.0,     1400.0  );

    this->_params[TuningParams::TUNE_HIST_COUNT].set(   "TUNE_HIST_COUNT",      true,   1.0677,     0.0,        2.0 );
    this->_params[TuningParams::TUNE_HIST_FOLLOW].set(  "TUNE_HIST_FOLLOW",     true,   0.9648	,   0.0,        2.0  );

    this->_params[TuningParams::TUNE_FUT_HIST_0].set(   "TUNE_FUT_HIST_0",      true,   11957.1174, 11000.0,    13000.0 );
    this->_params[TuningParams::TUNE_FUT_HIST_1].set(   "TUNE_FUT_HIST_1",      true,   6034.9722,  5400.0,     6600.0  );

    this->_params[TuningParams::TUNE_COUNT_DEPTH_0].set("TUNE_COUNT_DEPTH_0",   false,  3.0,        1.0,        5.0,    0.6     );
    this->_params[TuningParams::TUNE_COUNT_DEPTH_1].set("TUNE_COUNT_DEPTH_1",   false,  2.0,        1.0,        5.0,    0.6     );
    this->_params[TuningParams::TUNE_COUNT_HIST_0].set( "TUNE_COUNT_HIST_0",    true,   -1017.1300, -1150.0,    -900.0  );
    this->_params[TuningParams::TUNE_COUNT_HIST_1].set( "TUNE_COUNT_HIST_1",    true,   -2565.5060, -2800.0,    -2400.0 );

    this->_params[TuningParams::TUNE_SEE_KILL].set(     "TUNE_SEE_KILL",        true,   -16.3265,   -22.0,      -10.0   );
    this->_params[TuningParams::TUNE_SEE_QUIET].set(    "TUNE_SEE_QUIET",       true,   -66.0005,   -75.0,      -50.0   );
    this->_params[TuningParams::TUNE_SEE_DEPTH].set(    "TUNE_SEE_DEPTH",       true,   9.8137,     3.0,        15.0    );

    this->_params[TuningParams::TUNE_LMR_0_0].set(      "TUNE_LMR_0_0",         true,   1.8984,     1.8,        2.1     );
    this->_params[TuningParams::TUNE_LMR_0_1].set(      "TUNE_LMR_0_1",         true,   2.4070,     2.2,        2.8     );
    this->_params[TuningParams::TUNE_LMR_1_0].set(      "TUNE_LMR_1_0",         true,   3.9791,     3.5,        4.5     );
    this->_params[TuningParams::TUNE_LMR_1_1].set(      "TUNE_LMR_1_1",         true,   3.9686,     3.5,        4.5     );

    this->_params[TuningParams::TUNE_LMR_DEPTH_0].set(  "TUNE_LMR_DEPTH_0",     true,   0.3200,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_LMR_DEPTH_1].set(  "TUNE_LMR_DEPTH_1",     true,   2.4925,     2.0,        2.8     );

    this->_params[TuningParams::TUNE_ASP_DELTA].set(     "TUNE_ASP_DELTA",      true,   17.0950,    10,         30.0    );
    this->_params[TuningParams::TUNE_ASP_DELTA_INC].set( "TUNE_ASP_DELTA_INC",  true,   1.5298,     1.0,        4.0     );
    this->_params[TuningParams::TUNE_ASP_ALPHA].set(     "TUNE_ASP_ALPHA",      true,   0.9554,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_ASP_BETA].set(      "TUNE_ASP_BETA",       true,   0.4892,     0.0,        1.0     );

    this->_params[TuningParams::TUNE_BETA_DEPTH].set(    "TUNE_BETA_DEPTH",     true,   7.3221,     4.0,        20.0    );
    this->_params[TuningParams::TUNE_BETA_PRUN].set(     "TUNE_BETA_PRUN",      true,   76.3202,    65,         90.0    );
    this->_params[TuningParams::TUNE_ALPHA_PRUN].set(    "TUNE_ALPHA_PRUN",     false,  3009.9027,  2600.0,     3400.0  );

    this->_params[TuningParams::TUNE_NULL_MIN].set(      "TUNE_NULL_MIN",       true,   2.7447,     0.0,        10.0,   0.6     );
    this->_params[TuningParams::TUNE_NULL_REDUCTION].set("TUNE_NULL_REDUCTION", true,   5.3277,     0.0,        10.0,   0.6     );
    this->_params[TuningParams::TUNE_NULL_DIV_1].set(    "TUNE_NULL_DIV_1",     true,   200.3581,   175.0,      225.0   );
    this->_params[TuningParams::TUNE_NULL_DIV_2].set(    "TUNE_NULL_DIV_2",     true,   5.1510,     2.0,        10.0,   0.6     );

    this->_params[TuningParams::TUNE_PROBCUT_DEPTH].set( "TUNE_PROBCUT_DEPTH",  true,   5.6699,     2.0,        10.0,   0.6     );
    this->_params[TuningParams::TUNE_PROBCUT_BETA].set(  "TUNE_PROBCUT_BETA",   true,   116.1120,   95.0,       135.0   );

    this->_params[TuningParams::TUNE_IIR_PV_RED].set(    "TUNE_IIR_PV_RED",     true,   3.4374,     0.0,        5.0,    0.6     );
    this->_params[TuningParams::TUNE_IIR_CUT_DEPTH].set( "TUNE_IIR_CUT_DEPTH",  true,   7.7577,     5.0,        12.0,   0.6     );
    this->_params[TuningParams::TUNE_IIR_CUT_RED].set(   "TUNE_IIR_CUT_RED",    true,   1.8104,     0.0,        5.0,    0.6     );

    this->_params[TuningParams::TUNE_FUT_MARGIN_0].set(  "TUNE_FUT_MARGIN_0",   true,   90.6924,    75.0,       110.0   );
    this->_params[TuningParams::TUNE_FUT_MARGIN_1].set(  "TUNE_FUT_MARGIN_1",   true,   59.6986,    45.0,       80.0    );
    this->_params[TuningParams::TUNE_FUT_MARGIN_2].set(  "TUNE_FUT_MARGIN_2",   true,   158.6237,   140.0,      180.0   );

    this->_params[TuningParams::TUNE_SING_DEPTH_1].set(  "TUNE_SING_DEPTH_1",   true,   6.4726,     2.0,        12.0,   0.6     );
    this->_params[TuningParams::TUNE_SING_DEPTH_2].set(  "TUNE_SING_DEPTH_2",   true,   3.7551,     0.0,        10.0,   0.6     );
    this->_params[TuningParams::TUNE_SING_COEFF].set(    "TUNE_SING_COEFF",     true,   0.4667,     0.1,        1.0     );

    this->_params[TuningParams::TUNE_HIST_REDUCT].set(   "TUNE_HIST_REDUCT",    true,   5081.0327,  4400.0,     5600.0  );

    this->_params[TuningParams::TUNE_EVAL_DIVIDER].set(  "TUNE_EVAL_DIVIDER",   true,   502.0331,   400.0,      600.0   );

    this->_params[TuningParams::TUNE_TIME_MID].set(      "TUNE_TIME_MID",       true,   47.8367,    35,         60.0    );
    this->_params[TuningParams::TUNE_TIME_MID_VAL].set(  "TIME_MID_VAL",        true,   0.4857,     0.0,        1.0     );
    this->_params[TuningParams::TUNE_TIME_C_MIN].set(    "TUNE_TIME_C_MIN",     false,  25.0,       10,         50.0    );
    this->_params[TuningParams::TUNE_TIME_D_MIN].set(    "TUNE_TIME_D_MIN",     false,  50.0,       25,         75.0    );
    this->_params[TuningParams::TUNE_TIME_C_MAX].set(    "TUNE_TIME_C_MAX",     false,  25.0,       10,         50.0    );
    this->_params[TuningParams::TUNE_TIME_D_MAX].set(    "TUNE_TIME_D_MAX",     false,  8.0,        2,          40.0    );

    for (int type = 0; type < TuningParams::TUNE_END; type++)
        this->set((TuningParams::Type)type, this->_params[type].value);
}


