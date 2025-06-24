#include "neural.h"

#include "bitboards.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <immintrin.h>
#include <memory.h>

#ifdef USE_CNPY
#include <cnpy.h>
#endif

#ifdef USE_NN

#include <incbin.h>

#ifdef NN_FILE
INCBIN(Model, NN_FILE);
#endif

#endif

#define QUANTIZATION_COEFF_L1   (180)
#define QUANTIZATION_COEFF_L2   (512)
#define PSQT_COEFF              (64)


double EVAL_DIVIDER = 500.0;


static const std::vector<std::vector<int>> &PIECE_TABLE = { { 0, 10, 4, 3, 2, 1, 0 }, { 5, 11, 9, 8, 7, 6, 5 } };

static const int KING_TABLE[64] =
{
#if (K_SIZE == 16)
// Table-16 (Koivisto)
     0,  1,  2,  3,  -3,  -2,  -1,  -0,
     4,  5,  6,  7,  -7,  -6,  -5,  -4,
     8,  9, 10, 11, -11, -10,  -9,  -8,
     8,  9, 10, 11, -11, -10,  -9,  -8,
    12, 12, 13, 13, -13, -13, -12, -12,
    12, 12, 13, 13, -13, -13, -12, -12,
    14, 14, 15, 15, -15, -15, -14, -14,
    14, 14, 15, 15, -15, -15, -14, -14
#endif
};

#if defined(__AVX512F__)

using avx_register_type_8 = __m512i;
using avx_register_type_16 = __m512i;
using avx_register_type_32 = __m512i;
#define avx_madd_epi16(a, b) (_mm512_madd_epi16(a, b))
#define avx_add_epi32(a, b)     (_mm512_add_epi32(a, b))
#define avx_mullo_epi16(a, b)   (_mm512_mullo_epi16(a, b))
#define avx_add_epi16(a, b)     (_mm512_add_epi16(a, b))
#define avx_sub_epi16(a, b)     (_mm512_sub_epi16(a, b))
#define avx_max_epi16(a, b)     (_mm512_max_epi16(a, b))
#define avx_min_epi16(a, b)     (_mm512_min_epi16(a, b))
#define avx_set1_epi16(a)       (_mm512_set1_epi16(a))
#define avx_set1_epi32(a)       (_mm512_set1_epi32(a))

#elif defined(__AVX2__)

using avx_register_type_8 =  __m256i;
using avx_register_type_16 = __m256i;
using avx_register_type_32 = __m256i;
#define avx_madd_epi16(a, b)    (_mm256_madd_epi16(a, b))
#define avx_add_epi32(a, b)     (_mm256_add_epi32(a, b))
#define avx_mullo_epi16(a, b)   (_mm256_mullo_epi16(a, b))
#define avx_add_epi16(a, b)     (_mm256_add_epi16(a, b))
#define avx_sub_epi16(a, b)     (_mm256_sub_epi16(a, b))
#define avx_max_epi16(a, b)     (_mm256_max_epi16(a, b))
#define avx_min_epi16(a, b)     (_mm256_min_epi16(a, b))
#define avx_set1_epi16(a)       (_mm256_set1_epi16(a))
#define avx_set1_epi32(a)       (_mm256_set1_epi32(a))

#elif defined(__SSE2__)

using avx_register_type_8 =  __m128i;
using avx_register_type_16 = __m128i;
using avx_register_type_32 = __m128i;
#define avx_madd_epi16(a, b) (_mm_madd_epi16(a, b))
#define avx_add_epi32(a, b)  (_mm_add_epi32(a, b))
#define avx_mullo_epi16(a, b)(_mm_mullo_epi16(a, b))
#define avx_add_epi16(a, b)  (_mm_add_epi16(a, b))
#define avx_sub_epi16(a, b)  (_mm_sub_epi16(a, b))
#define avx_max_epi16(a, b)  (_mm_max_epi16(a, b))
#define avx_min_epi16(a, b)  (_mm_min_epi16(a, b))
#define avx_set1_epi16(a)    (_mm_set1_epi16(a))
#define avx_set1_epi32(a)    (_mm_set1_epi32(a))

#endif

Model &Model::instance()
{
    static Model theSingleInstance;
    return theSingleInstance;
}

void Model::init(std::string filename)
{
#ifdef USE_NN
    this->free();

    this->_l1bias = new (std::align_val_t(ALIGNMENT)) float[L1_OUT_SIZE_P];
    this->_l1data = new (std::align_val_t(ALIGNMENT)) float[IN_SIZE * L1_OUT_SIZE_P];
    this->_l2bias = new (std::align_val_t(ALIGNMENT)) float[HIDDEN2_SIZE];
    this->_l2data = new (std::align_val_t(ALIGNMENT)) float[HIDDEN_SIZE2 * HIDDEN2_SIZE];

    this->_l1bias_avx = new (std::align_val_t(ALIGNMENT)) i16[L1_OUT_SIZE_P2];
    this->_l1data_avx = new (std::align_val_t(ALIGNMENT)) i16[IN_SIZE * L1_OUT_SIZE_P];
    this->_l2bias_avx = new (std::align_val_t(ALIGNMENT)) i32[HIDDEN2_SIZE];
    this->_l2data_avx = new (std::align_val_t(ALIGNMENT)) i16[HIDDEN_SIZE2 * HIDDEN2_SIZE];

    if (filename == "")
    {
#ifdef NN_FILE
        std::cout << "info string Loading model (incbin): " << NN_FILE << std::endl;
        int idx = 0;
        memcpy(this->_l1bias, &gModelData[idx], L1_OUT_SIZE_P * sizeof(float));
        idx += L1_OUT_SIZE_P * sizeof(float);
        memcpy(this->_l1data, &gModelData[idx], IN_SIZE * L1_OUT_SIZE_P * sizeof(float));
        idx += IN_SIZE * L1_OUT_SIZE_P * sizeof(float);
        memcpy(this->_l2bias, &gModelData[idx], HIDDEN2_SIZE * sizeof(float));
        idx += HIDDEN2_SIZE * sizeof(float);
        memcpy(this->_l2data, &gModelData[idx], HIDDEN_SIZE2 * HIDDEN2_SIZE * sizeof(float));
        idx += HIDDEN_SIZE2 * HIDDEN2_SIZE * sizeof(float);
        if (idx != gModelSize)
            std::cout << "if (idx != gModelSize) --- idx: " << idx << " --- gModelSize: " << gModelSize << std::endl;
#endif  // NN_FILE
    }
    else
    {
        std::cout << "info string Loading model: " << filename << std::endl;
        std::fstream file(filename, std::ios::in | std::ios::binary);
        file.read((char *)this->_l1bias, L1_OUT_SIZE_P * sizeof(float));
        file.read((char *)this->_l1data, IN_SIZE * L1_OUT_SIZE_P * sizeof(float));
        file.read((char *)this->_l2bias, HIDDEN2_SIZE * sizeof(float));
        file.read((char *)this->_l2data, HIDDEN_SIZE2 * HIDDEN2_SIZE * sizeof(float));
        file.close();
    }

    // L1 avx
    for (int i = 0; i < L1_OUT_SIZE_P; ++i)
    {
        float quant = this->_l1bias[i] * (i < HIDDEN_SIZE ? QUANTIZATION_COEFF_L1 : PSQT_COEFF);
        this->_l1bias_avx[i] = std::round(quant);
    }
    for (int i = 0; i < IN_SIZE; ++i)
        for (int j = 0; j < L1_OUT_SIZE_P; ++j)
        {
            int idx = i * L1_OUT_SIZE_P + j;
            float quant = this->_l1data[idx] * (j < HIDDEN_SIZE ? QUANTIZATION_COEFF_L1 : PSQT_COEFF);
            this->_l1data_avx[idx] = std::round(quant);
        }

    // L2 avx
    for (int i = 0; i < HIDDEN2_SIZE; ++i)
    {
        float quant = this->_l2bias[i] * QUANTIZATION_COEFF_L1 * QUANTIZATION_COEFF_L2;
#ifdef SCRELU
        quant *= QUANTIZATION_COEFF_L1;
#endif
        this->_l2bias_avx[i] = std::round(quant);
    }
    for (int i = 0; i < HIDDEN_SIZE2 * HIDDEN2_SIZE; ++i)
    {
        float quant = this->_l2data[i] * QUANTIZATION_COEFF_L2;
        this->_l2data_avx[i] = std::round(quant);
    }

    // L1 bias copy
    for (int i = 0; i < L1_OUT_SIZE_P; ++i)
        this->_l1bias_avx[L1_OUT_SIZE_P+i] = this->_l1bias_avx[i];

#else   // USE_NN

    std::cout << "Neural Network disabled :(" << std::endl;

#endif
}

void Model::free()
{
    if (this->_l1bias)
        ::operator delete[](this->_l1bias, std::align_val_t(ALIGNMENT));
    this->_l1bias = nullptr;
    if (this->_l1data)
        ::operator delete[](this->_l1data, std::align_val_t(ALIGNMENT));
    this->_l1data = nullptr;
    if (this->_l2bias)
        ::operator delete[](this->_l2bias, std::align_val_t(ALIGNMENT));
    this->_l2bias = nullptr;
    if (this->_l2data)
        ::operator delete[](this->_l2data, std::align_val_t(ALIGNMENT));
    this->_l2data = nullptr;

    if (this->_l1bias_avx)
        ::operator delete[](this->_l1bias_avx, std::align_val_t(ALIGNMENT));
    this->_l1bias_avx = nullptr;
    if (this->_l1data_avx)
        ::operator delete[](this->_l1data_avx, std::align_val_t(ALIGNMENT));
    this->_l1data_avx = nullptr;
    if (this->_l2bias_avx)
        ::operator delete[](this->_l2bias_avx, std::align_val_t(ALIGNMENT));
    this->_l2bias_avx = nullptr;
    if (this->_l2data_avx)
        ::operator delete[](this->_l2data_avx, std::align_val_t(ALIGNMENT));
    this->_l2data_avx = nullptr;
}

Model::Model()
{
    this->_l1bias = nullptr;
    this->_l1data = nullptr;
    this->_l2bias = nullptr;
    this->_l2data = nullptr;

    this->_l1bias_avx = nullptr;
    this->_l1data_avx = nullptr;
    this->_l2bias_avx = nullptr;
    this->_l2data_avx = nullptr;
}

Model::~Model()
{
    this->free();
}

Neural::Neural()
{
    this->stack_clear();
}

Neural::~Neural()
{
}

void Neural::stack_clear()
{
    this->_pointer = 0;
    auto stack = &this->_stack[this->_pointer];
    stack->accurate_point_w = 0;
    stack->accurate_point_b = 0;
    stack->add_w[0] = 0;
    stack->add_b[0] = 0;
    stack->remove_w[0] = 0;
    stack->remove_b[0] = 0;
    stack->refresh_w = false;
    stack->refresh_b = false;
}

void Neural::stack_push()
{
    auto stack_prev = &this->_stack[this->_pointer];
    this->_pointer++;
    auto stack = &this->_stack[this->_pointer];
    stack->accurate_point_w = stack_prev->accurate_point_w;
    stack->accurate_point_b = stack_prev->accurate_point_b;
    stack->add_w[0] = 0;
    stack->add_b[0] = 0;
    stack->remove_w[0] = 0;
    stack->remove_b[0] = 0;
    stack->refresh_w = false;
    stack->refresh_b = false;
}

void Neural::stack_pull()
{
    this->_pointer--;
}

void Neural::stack_pull_copy()
{
    this->accum_lazy_update();
    this->_pointer--;
    this->accum_copy(this->_pointer+1, this->_pointer, true, true);
}

void Neural::accum_init(bool do_w, bool do_b)
{
    Model &model = Model::instance();
    if (do_w && do_b)
        memcpy(this->_stack[this->_pointer]._layer1, model._l1bias_avx, L1_OUT_SIZE_P2 * sizeof(i16));
    else if (do_w)
        memcpy(this->_stack[this->_pointer]._layer1, model._l1bias_avx, L1_OUT_SIZE_P * sizeof(i16));
    else if (do_b)
        memcpy(&this->_stack[this->_pointer]._layer1[L1_OUT_SIZE_P], &model._l1bias_avx[L1_OUT_SIZE_P], L1_OUT_SIZE_P * sizeof(i16));
}

void Neural::accum_lazy_delta_add(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b)
{
    auto stack = &this->_stack[this->_pointer];
    if (do_w)
        stack->add_w[++stack->add_w[0]] = this->idx_w(wk, color, piece, sq) * L1_SIZE;
    if (do_b)
        stack->add_b[++stack->add_b[0]] = this->idx_b(bk, color, piece, sq) * L1_SIZE;
}

void Neural::accum_lazy_delta_remove(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b)
{
    auto stack = &this->_stack[this->_pointer];
    if (do_w)
        stack->remove_w[++stack->remove_w[0]] = this->idx_w(wk, color, piece, sq) * L1_SIZE;
    if (do_b)
        stack->remove_b[++stack->remove_b[0]] = this->idx_b(bk, color, piece, sq) * L1_SIZE;
}

void Neural::accum_lazy_refresh(u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq, bool do_w, bool do_b)
{
    int wk_pos = Bitboards::lsb(wk);
    int bk_pos = Bitboards::lsb(bk);

    // PAWNS
    while (wp != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 0, 6, Bitboards::poplsb(wp), do_w, do_b);
    while (bp != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 1, 6, Bitboards::poplsb(bp), do_w, do_b);

    // KNIGHTS
    while (wn != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 0, 5, Bitboards::poplsb(wn), do_w, do_b);
    while (bn != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 1, 5, Bitboards::poplsb(bn), do_w, do_b);

    // BISHOPS
    while (wb != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 0, 4, Bitboards::poplsb(wb), do_w, do_b);
    while (bb != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 1, 4, Bitboards::poplsb(bb), do_w, do_b);

    // ROOKS
    while (wr != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 0, 3, Bitboards::poplsb(wr), do_w, do_b);
    while (br != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 1, 3, Bitboards::poplsb(br), do_w, do_b);

    // QUEENS
    while (wq != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 0, 2, Bitboards::poplsb(wq), do_w, do_b);
    while (bq != 0)
        accum_lazy_delta_add(wk_pos, bk_pos, 1, 2, Bitboards::poplsb(bq), do_w, do_b);

    // KINGS
    accum_lazy_delta_add(wk_pos, bk_pos, 0, 1, wk_pos, do_w, do_b);
    accum_lazy_delta_add(wk_pos, bk_pos, 1, 1, bk_pos, do_w, do_b);

    auto stack = &this->_stack[this->_pointer];
    if (do_w)
    {
        stack->refresh_w = true;
        stack->accurate_point_w = this->_pointer - 1;
    }
    if (do_b)
    {
        stack->refresh_b = true;
        stack->accurate_point_b = this->_pointer - 1;
    }
}

void Neural::accum_lazy_update()
{
    Model &model = Model::instance();
    const auto l1bias = (avx_register_type_16*) (model._l1bias_avx);
    const auto l1data = (avx_register_type_16*) (model._l1data_avx);

    avx_register_type_16 regs[NUM_REGS];

    auto stack = &this->_stack[this->_pointer];

    auto stack_prev = &this->_stack[stack->accurate_point_w];
    for (int idx = stack->accurate_point_w + 1; idx <= this->_pointer; ++idx)
    {
        auto stack_idx = &this->_stack[idx];

        const auto sum = (avx_register_type_16*) (stack_idx->_layer1);
        const auto sum_prev = (avx_register_type_16*) (stack_prev->_layer1);

        for (int i = 0; i < L1_SIZE; i += NUM_REGS)
        {
            for (int r = 0; r < NUM_REGS; ++r)
                regs[r] = stack_idx->refresh_w ? l1bias[i+r] : sum_prev[i+r];

            for (int j = 1; j <= stack_idx->add_w[0]; ++j)
                for (int r = 0; r < NUM_REGS; ++r)
                    regs[r] = avx_add_epi16(regs[r], l1data[stack_idx->add_w[j] + i+r]);
            for (int j = 1; j <= stack_idx->remove_w[0]; ++j)
                for (int r = 0; r < NUM_REGS; ++r)
                    regs[r] = avx_sub_epi16(regs[r], l1data[stack_idx->remove_w[j] + i+r]);

            for (int r = 0; r < NUM_REGS; ++r)
                sum[i+r] = regs[r];
        }

        stack_idx->accurate_point_w = idx;

        stack_prev = stack_idx;
    }

    stack_prev = &this->_stack[stack->accurate_point_b];
    for (int idx = stack->accurate_point_b + 1; idx <= this->_pointer; ++idx)
    {
        auto stack_idx = &this->_stack[idx];

        const auto sum = (avx_register_type_16*) (stack_idx->_layer1);
        const auto sum_prev = (avx_register_type_16*) (stack_prev->_layer1);

        for (int i = 0; i < L1_SIZE; i += NUM_REGS)
        {
            for (int r = 0; r < NUM_REGS; ++r)
                regs[r] = stack_idx->refresh_b ? l1bias[i+r] : sum_prev[i+r + L1_SIZE];

            for (int j = 1; j <= stack_idx->add_b[0]; ++j)
                for (int r = 0; r < NUM_REGS; ++r)
                    regs[r] = avx_add_epi16(regs[r], l1data[stack_idx->add_b[j] + i+r]);
            for (int j = 1; j <= stack_idx->remove_b[0]; ++j)
                for (int r = 0; r < NUM_REGS; ++r)
                    regs[r] = avx_sub_epi16(regs[r], l1data[stack_idx->remove_b[j] + i+r]);

            for (int r = 0; r < NUM_REGS; ++r)
                sum[i+r + L1_SIZE] = regs[r];
        }

        stack_idx->accurate_point_b = idx;

        stack_prev = stack_idx;
    }
}

void Neural::accum_copy(int from, int to, bool do_w, bool do_b)
{
    if (do_w && do_b)
        memcpy(this->_stack[to]._layer1, this->_stack[from]._layer1, L1_OUT_SIZE_P2 * sizeof(i16));
    else if (do_w)
        memcpy(this->_stack[to]._layer1, this->_stack[from]._layer1, L1_OUT_SIZE_P * sizeof(i16));
    else if (do_b)
        memcpy(&this->_stack[to]._layer1[L1_OUT_SIZE_P], &this->_stack[from]._layer1[L1_OUT_SIZE_P], L1_OUT_SIZE_P * sizeof(i16));
}

int Neural::idx_w(int wk, int color, int piece, int sq)
{
    int wk_side = ((wk & 7) > 3) ? 7 : 0;
    return KING_TABLE[wk ^ wk_side] * P_SIZE + (sq ^ wk_side) + PIECE_TABLE[color][piece] * 64;
}

int Neural::idx_b(int bk, int color, int piece, int sq)
{
    int bk_side = ((bk & 7) > 3) ? 7 : 0;
    return KING_TABLE[bk ^ 56 ^ bk_side] * P_SIZE + (sq ^ 56 ^ bk_side) + PIECE_TABLE[1-color][piece] * 64;

}

void Neural::accum_piece_add(int wk, int bk, int color, int piece, int sq, bool do_w, bool do_b)
{
    Model &model = Model::instance();

    int w_pos = idx_w(wk, color, piece, sq) * L1_SIZE;
    int b_pos = idx_b(bk, color, piece, sq) * L1_SIZE;

    const auto l1data = (avx_register_type_16*) (model._l1data_avx);
    const auto sum = (avx_register_type_16*) (this->_stack[this->_pointer]._layer1);

    if (do_w)
        for (int i = 0; i < L1_SIZE; ++i)
            sum[i] = avx_add_epi16(sum[i], l1data[w_pos+i]);
    if (do_b)
        for (int i = 0; i < L1_SIZE; ++i)
            sum[i + L1_SIZE] = avx_add_epi16(sum[i + L1_SIZE], l1data[b_pos+i]);
}

void Neural::accum_all_pieces(u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq, bool do_w, bool do_b)
{
    accum_init(do_w, do_b);

    int wk_pos = Bitboards::lsb(wk);
    int bk_pos = Bitboards::lsb(bk);

    // PAWNS
    while (wp != 0)
        accum_piece_add(wk_pos, bk_pos, 0, 6, Bitboards::poplsb(wp), do_w, do_b);
    while (bp != 0)
        accum_piece_add(wk_pos, bk_pos, 1, 6, Bitboards::poplsb(bp), do_w, do_b);

    // KNIGHTS
    while (wn != 0)
        accum_piece_add(wk_pos, bk_pos, 0, 5, Bitboards::poplsb(wn), do_w, do_b);
    while (bn != 0)
        accum_piece_add(wk_pos, bk_pos, 1, 5, Bitboards::poplsb(bn), do_w, do_b);

    // BISHOPS
    while (wb != 0)
        accum_piece_add(wk_pos, bk_pos, 0, 4, Bitboards::poplsb(wb), do_w, do_b);
    while (bb != 0)
        accum_piece_add(wk_pos, bk_pos, 1, 4, Bitboards::poplsb(bb), do_w, do_b);

    // ROOKS
    while (wr != 0)
        accum_piece_add(wk_pos, bk_pos, 0, 3, Bitboards::poplsb(wr), do_w, do_b);
    while (br != 0)
        accum_piece_add(wk_pos, bk_pos, 1, 3, Bitboards::poplsb(br), do_w, do_b);

    // QUEENS
    while (wq != 0)
        accum_piece_add(wk_pos, bk_pos, 0, 2, Bitboards::poplsb(wq), do_w, do_b);
    while (bq != 0)
        accum_piece_add(wk_pos, bk_pos, 1, 2, Bitboards::poplsb(bq), do_w, do_b);

    // KINGS
    accum_piece_add(wk_pos, bk_pos, 0, 1, wk_pos, do_w, do_b);
    accum_piece_add(wk_pos, bk_pos, 1, 1, bk_pos, do_w, do_b);

    auto stack = &this->_stack[this->_pointer];
    if (do_w)
        stack->accurate_point_w = this->_pointer;
    if (do_b)
        stack->accurate_point_b = this->_pointer;
}

int Neural::accum_predict(int color, int stage)
{
    this->accum_lazy_update();

    float res = color ?
                    this->l2_predict(&this->_stack[this->_pointer]._layer1[L1_OUT_SIZE_P], &this->_stack[this->_pointer]._layer1[0], stage) :
                    this->l2_predict(&this->_stack[this->_pointer]._layer1[0], &this->_stack[this->_pointer]._layer1[L1_OUT_SIZE_P], stage);

    if (color)
        res = -res;

    return static_cast<int>(res);
}

int Neural::predict_i(int color, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq)
{
    this->stack_clear();
    this->accum_all_pieces(wk, bk, wp, bp, wn, bn, wb, bb, wr, br, wq, bq, true, true);
    int stg = Neural::stage(Bitboards::bits_count(wk | bk | wp | bp | wn | bn | wb | bb | wr | br | wq | bq), (wq | bq) != 0);
    return accum_predict(color, stg);
}

float Neural::sigmoid(float data)
{
    return 1.0f / (1.0f + std::exp(-data));
}

int Neural::king_area(int sq, int color)
{
    return color ? KING_TABLE[sq ^ 56] : KING_TABLE[sq];
}

int Neural::stage(int pieces_count, bool is_queens)
{
#if (OUT_SIZE == 1)
    return 0;
#elif (OUT_SIZE == 6)
    return std::max(0, (pieces_count - 3) / 5);
#else
    return 0;
#endif
}

float Neural::l2_predict(i16 *to_move, i16 *opponent, int stage)
{
    Model &model = Model::instance();

    int l2_idx = stage * HIDDEN_SIZE2/COUNT_16_BIT;

    const auto l1_tomove = (avx_register_type_16*) (to_move);
    const auto l1_opp = (avx_register_type_16*) (opponent);
    const auto l2data = (avx_register_type_16*) (model._l2data_avx);

    avx_register_type_32 sum = avx_set1_epi32(0);
    avx_register_type_16 zeros = avx_set1_epi16(0);
    avx_register_type_16 ones = avx_set1_epi16(QUANTIZATION_COEFF_L1);

    for (int i = 0; i < (HIDDEN_SIZE2 / 2)/COUNT_16_BIT; i++)
    {
        auto crelu1a = avx_min_epi16(avx_max_epi16(l1_tomove[i], zeros), ones);
        auto crelu2a = avx_min_epi16(avx_max_epi16(l1_opp[i], zeros), ones);
#ifdef IS_PM
        auto crelu1b = avx_min_epi16(avx_max_epi16(l1_tomove[i + (HIDDEN_SIZE / 2)/COUNT_16_BIT], zeros), ones);
        auto crelu2b = avx_min_epi16(avx_max_epi16(l1_opp[i + (HIDDEN_SIZE / 2)/COUNT_16_BIT], zeros), ones);

        sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu1a, crelu1b), l2data[l2_idx + i]));
        sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu2a, crelu2b), l2data[l2_idx + (HIDDEN_SIZE / 2)/COUNT_16_BIT + i]));
#else
        // sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu1a, l2data[l2_idx + i]), crelu1a));
        // sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu2a, l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]), crelu2a));
        sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu1a, crelu1a), l2data[l2_idx + i]));
        sum = avx_add_epi32(sum, avx_madd_epi16(avx_mullo_epi16(crelu2a, crelu2a), l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]));
#endif
    }

#if defined(__AVX512F__)
    const __m256i reduced_8 = _mm256_add_epi32(_mm512_castsi512_si256(sum), _mm512_extracti32x8_epi32(sum, 1));
#elif defined(__AVX2__)
    const __m256i reduced_8 = sum;
#endif

#if defined(__AVX512F__) || defined(__AVX2__)
    const __m128i reduced_4 = _mm_add_epi32(_mm256_castsi256_si128(reduced_8), _mm256_extractf128_si256(reduced_8, 1));
#elif defined(__SSE2__)
    const __m128i reduced_4 = sum;
#endif

    __m128i vsum = _mm_add_epi32(reduced_4, _mm_srli_si128(reduced_4, 8));
    vsum         = _mm_add_epi32(vsum, _mm_srli_si128(vsum, 4));

    float res = _mm_cvtsi128_si32(vsum) + (int)model._l2bias_avx[stage];
#ifdef SCRELU
    res *= EVAL_DIVIDER / (QUANTIZATION_COEFF_L1*QUANTIZATION_COEFF_L1*QUANTIZATION_COEFF_L2);
#else
    res *= EVAL_DIVIDER / (QUANTIZATION_COEFF_L1*QUANTIZATION_COEFF_L2);
#endif

#ifdef PADDING
    int psqt = (to_move[HIDDEN_SIZE + stage] - opponent[HIDDEN_SIZE + stage]) / 2;
    res += psqt;
#endif
    return res;
}
