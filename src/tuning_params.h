#ifndef TUNING_PARAMS_H
#define TUNING_PARAMS_H

#include <string>
#include <vector>

struct Param
{
    std::string name;
    bool need_tuning;
    double value;
    double min;
    double max;
    double c_end;
    double r_end;

    void set(std::string n, bool t, double v, double mi, double ma, double c = -1.0);
};

class TuningParams
{
public:
    enum Type
    {
        TUNE_SEE_PAWN = 0,
        TUNE_SEE_KNIGHT,
        TUNE_SEE_BISHOP,
        TUNE_SEE_ROOK,
        TUNE_SEE_QUEEN,

        TUNE_HIST_COUNT,
        TUNE_HIST_FOLLOW,

        TUNE_FUT_HIST_0,
        TUNE_FUT_HIST_1,

        TUNE_COUNT_DEPTH_0,
        TUNE_COUNT_DEPTH_1,
        TUNE_COUNT_HIST_0,
        TUNE_COUNT_HIST_1,

        TUNE_SEE_KILL,
        TUNE_SEE_QUIET,
        TUNE_SEE_DEPTH,

        TUNE_LMR_0_0,
        TUNE_LMR_0_1,
        TUNE_LMR_1_0,
        TUNE_LMR_1_1,

        TUNE_LMR_DEPTH_0,
        TUNE_LMR_DEPTH_1,

        TUNE_ASP_DELTA,
        TUNE_ASP_DELTA_INC,
        TUNE_ASP_ALPHA,
        TUNE_ASP_BETA,

        TUNE_BETA_DEPTH,
        TUNE_BETA_PRUN,
        TUNE_BETA_IMPROV_0,
        TUNE_BETA_IMPROV_1,
        TUNE_BETA_HASHHIT_0,
        TUNE_BETA_HASHHIT_1,
        TUNE_BETA_RETURN,

        // TUNE_ALPHA_PRUN,

        TUNE_NULL_MIN,
        TUNE_NULL_REDUCTION,
        TUNE_NULL_DIV_1,
        TUNE_NULL_DIV_2,

        TUNE_PROBCUT_DEPTH,
        TUNE_PROBCUT_BETA,

        TUNE_IIR_PV_RED,
        TUNE_IIR_CUT_DEPTH,
        TUNE_IIR_CUT_RED,

        TUNE_FUT_MARGIN_0,
        TUNE_FUT_MARGIN_1,
        TUNE_FUT_MARGIN_2,

        TUNE_SING_DEPTH_1,
        TUNE_SING_DEPTH_2,
        TUNE_SING_COEFF,
        TUNE_SING_EXTS,
        // TUNE_SING_BETA,

        TUNE_HIST_REDUCT,

        TUNE_EVAL_DIVIDER,

        TUNE_TIME_MID,
        TUNE_TIME_MID_VAL,
        TUNE_TIME_C_MIN,
        TUNE_TIME_D_MIN,
        TUNE_TIME_C_MAX,
        TUNE_TIME_D_MAX,

        TUNE_END
    };

    static TuningParams &instance();

    void set(Type type, double value);
    bool set(const std::string &name, double value);

    std::vector<Param> _params;

private:
    TuningParams();
    TuningParams(const TuningParams& root) = delete;
    TuningParams& operator=(const TuningParams&) = delete;
};

#endif // TUNING_PARAMS_H
