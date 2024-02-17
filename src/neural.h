#ifndef NEURAL_H
#define NEURAL_H

#include <string>

#include "common.h"

#define K_SIZE          (16)
#define P_SIZE          (768)
#define IN_SIZE         (K_SIZE*P_SIZE)
#define HIDDEN_SIZE     (1024)
#define HIDDEN_SIZE2    (HIDDEN_SIZE * 2)
#define OUT_SIZE        (6)
#define PADDING         (32-OUT_SIZE)
#define L1_OUT_SIZE     (HIDDEN_SIZE + OUT_SIZE)
#define L1_OUT_SIZE_P   (L1_OUT_SIZE + PADDING)
//#define L1_OUT_SIZE     (HIDDEN_SIZE)
//#define L1_OUT_SIZE_P   (L1_OUT_SIZE)
#define L1_OUT_SIZE_P2  (L1_OUT_SIZE_P * 2)

#define USE_INTRIN  // fix for case when OUT_SIZE != 1
//#define CLIPPED_RELU

#if defined(__AVX512F__)
#define BIT_ALIGNMENT   (512)
#define UNROLL          (256)
#elif defined(__AVX2__)
#define BIT_ALIGNMENT   (256)
#define UNROLL          (128)
#elif defined(__SSE2__)
#define BIT_ALIGNMENT   (128)
#define UNROLL          (64)
#endif
#define COUNT_16_BIT   (BIT_ALIGNMENT / 16)
#define ALIGNMENT      (BIT_ALIGNMENT / 8)

struct Accumulator
{
    alignas(ALIGNMENT) i16 _layer1[L1_OUT_SIZE_P2];
} __attribute__((aligned(2048)));

class Model
{
public:
    static Model& instance();

    void init(std::string filename = "");
    void free();

    i16 *_l1bias_avx;
    i16 *_l1data_avx;
    i16 *_l2bias_avx;
    i16 *_l2data_avx;

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

    void accum_init(int stack_pointer);
    void accum_copy(int from, int to);
    void accum_piece_add(int stack_pointer, int wk, int bk, int color, int piece, int sq);
    void accum_piece_remove(int stack_pointer, int wk, int bk, int color, int piece, int sq);
    void accum_all_pieces(int stack_pointer, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq);
    int accum_predict(int stack_pointer, int color, int stage);

    int predict_i(int color, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq);
    static float sigmoid(float data);
    static int king_area(int sq, int color);
    static int stage(int pieces_count, bool is_queens);

private:
    Accumulator _stack[128];
};

extern double EVAL_DIVIDER;

#endif // NEURAL_H
