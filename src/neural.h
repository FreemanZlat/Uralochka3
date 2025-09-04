#ifndef NEURAL_H
#define NEURAL_H

#include <string>

#include "common.h"

#define K_SIZE          (16)
#define P_SIZE          (768)
#define IN_SIZE         (K_SIZE*P_SIZE)
#define HIDDEN_SIZE     (1280)

#define OUT_SIZE        (6)                 // S_SIZE

#define PADDING         (32-OUT_SIZE)       // PSQT

#define HIDDEN2_SIZE    (6)
#define HIDDEN3_SIZE    (32)

#ifdef PADDING
#define L1_OUT_SIZE     (HIDDEN_SIZE + OUT_SIZE)
#define L1_OUT_SIZE_P   (L1_OUT_SIZE + PADDING)
#else
#define L1_OUT_SIZE     (HIDDEN_SIZE)
#define L1_OUT_SIZE_P   (L1_OUT_SIZE)
#endif

#define L1_OUT_SIZE_P2  (L1_OUT_SIZE_P * 2)

#define L2_OUT_SIZE     (HIDDEN2_SIZE * OUT_SIZE)
#define L3_OUT_SIZE     (HIDDEN3_SIZE * OUT_SIZE)

#if defined(__AVX512F__)
#define BIT_ALIGNMENT   (512)
#define NUM_REGS_1     (16)
#define NUM_REGS_2     (9)
#elif defined(__AVX2__)
#define BIT_ALIGNMENT   (256)
#define NUM_REGS_1     (14)
#define NUM_REGS_2     (12)
#elif defined(__SSE2__)
#define BIT_ALIGNMENT   (128)
#define NUM_REGS_1     (8)
#define NUM_REGS_2     (4)
#endif
#define COUNT_32_BIT   (BIT_ALIGNMENT / 32)
#define COUNT_16_BIT   (BIT_ALIGNMENT / 16)
#define COUNT_8_BIT    (BIT_ALIGNMENT / 8)
#define ALIGNMENT      (BIT_ALIGNMENT / 8)

#define L1_SIZE        (L1_OUT_SIZE_P/COUNT_16_BIT)

struct Accumulator
{
    alignas(ALIGNMENT) i16 _layer1[L1_OUT_SIZE_P2];
    int accurate_point_w;
    int accurate_point_b;
    int add_w[4];
    int add_b[4];
    int remove_w[4];
    int remove_b[4];
};

struct FinnyNode
{
    u64 _bitboards[2][7];
    alignas(ALIGNMENT) i16 _layer1[L1_OUT_SIZE_P];
};

class Model
{
public:
    static Model& instance();

    void init(std::string filename = "");
    void free();

    float *_l1bias;
    float *_l1data;
    float *_l2bias;
    float *_l2data;
    float *_l3bias;
    float *_l3data;
    float *_l4bias;
    float *_l4data;

    i16 *_l1bias_avx;
    i16 *_l1data_avx;
    i32 *_l2bias_avx;
    i16 *_l2data_avx;   // !!!

private:
    Model();
    ~Model();
    Model(const Model &) = delete;
    Model& operator=(const Model &) = delete;
};

class Neural
{
public:
    Neural();
    ~Neural();

    void stack_clear();
    void stack_push();
    void stack_pull();
    void stack_pull_copy();

    void accum_init(bool do_w, bool do_b);
    void accum_lazy_delta_add(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b);
    void accum_lazy_delta_remove(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b);
    void accum_lazy_refresh(u64 (*bbs)[7], bool do_w, bool do_b);
    void accum_finny_update(int color, int wk, int bk, u64 (*bbs)[7], FinnyNode *node);
    void accum_lazy_update();
    void accum_piece_add(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b);
    void accum_all_pieces(u64 (*bbs)[7], bool do_w, bool do_b);
    int accum_predict(int color, int stage);

    int predict_i(int color, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq);
    static float sigmoid(float data);
    static int king_area(int sq, int color);
    static int stage(int pieces_count);

private:
    Accumulator _stack[128];
    int _pointer;

    FinnyNode _finny[2][K_SIZE*2];

    void accum_copy(int from, int to, bool do_w, bool do_b);

    int idx_w(int wk, int color, int piece, int sq);
    int idx_b(int bk, int color, int piece, int sq);
    int idx_finny(int pos, int color);

    float l234_predict(i16 *to_move, i16 *opponent, int stage);
};

extern double EVAL_DIVIDER;

#endif // NEURAL_H
