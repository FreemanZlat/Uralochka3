#ifndef NEURAL_H
#define NEURAL_H

#include <string>

#include "common.h"

#define IS_PM

#define K_SIZE          (16)
#define P_SIZE          (768)
#define IN_SIZE         (K_SIZE*P_SIZE)
#define HIDDEN_SIZE     (1024)

#ifdef IS_PM
#define HIDDEN_SIZE2    (HIDDEN_SIZE)
#else
#define HIDDEN_SIZE2    (HIDDEN_SIZE * 2)
#endif

#define OUT_SIZE        (6)                 // S_SIZE

#define PADDING         (32-OUT_SIZE)       // PSQT

#define HIDDEN2_SIZE    (OUT_SIZE)

#ifdef PADDING
#define L1_OUT_SIZE     (HIDDEN_SIZE + OUT_SIZE)
#define L1_OUT_SIZE_P   (L1_OUT_SIZE + PADDING)
#else
#define L1_OUT_SIZE     (HIDDEN_SIZE)
#define L1_OUT_SIZE_P   (L1_OUT_SIZE)
#endif

#define L1_OUT_SIZE_P2  (L1_OUT_SIZE_P * 2)

#define CLIPPED_RELU
#define SCRELU

#if defined(__AVX512F__)
#define BIT_ALIGNMENT   (512)
#elif defined(__AVX2__)
#define BIT_ALIGNMENT   (256)
#elif defined(__SSE2__)
#define BIT_ALIGNMENT   (128)
#endif
#define COUNT_32_BIT   (BIT_ALIGNMENT / 32)
#define COUNT_16_BIT   (BIT_ALIGNMENT / 16)
#define COUNT_8_BIT    (BIT_ALIGNMENT / 8)
#define ALIGNMENT      (BIT_ALIGNMENT / 8)

#define L1_SIZE         (L1_OUT_SIZE_P/COUNT_16_BIT)

#define NUM_REGS        (11)

struct Accumulator
{
    alignas(ALIGNMENT) i16 _layer1[L1_OUT_SIZE_P2];
    int accurate_point_w;
    int accurate_point_b;
    int add_w[34];
    int add_b[34];
    int remove_w[4];
    int remove_b[4];
    int refresh_w;
    int refresh_b;
} __attribute__((aligned(2048)));

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

    i16 *_l1bias_avx;
    i16 *_l1data_avx;
    i32 *_l2bias_avx;
    i16 *_l2data_avx;   // may be i32!!!

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
    void accum_lazy_refresh(u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq, bool do_w, bool do_b);
    void accum_lazy_update();
    void accum_piece_add(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b);
    void accum_all_pieces(u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq, bool do_w, bool do_b);
    int accum_predict(int color, int stage);

    int predict_i(int color, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq);
    static float sigmoid(float data);
    static int king_area(int sq, int color);
    static int stage(int pieces_count, bool is_queens);

private:
    Accumulator _stack[128];
    int _pointer;

    void accum_copy(int from, int to, bool do_w, bool do_b);

    int idx_w(int wk, int color, int piece, int sq);
    int idx_b(int bk, int color, int piece, int sq);

    float l2_predict(i16 *to_move, i16 *opponent, int stage);
};

extern double EVAL_DIVIDER;

#endif // NEURAL_H
