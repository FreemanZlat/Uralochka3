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

#define QUANTIZATION_COEFF_L1   (64)
#define QUANTIZATION_COEFF_L2   (512)

static const std::vector<std::vector<int>> &PIECE_TABLE = { { 0, 10, 4, 3, 2, 1, 0 }, { 5, 11, 9, 8, 7, 6, 5 } };

static const int KING_TABLE[64] =
{
#if (K_SIZE == 10)
// Table-10
    0,  1,  2,  3,  3,  2,  1,  0,
    4,  4,  5,  5,  5,  5,  4,  4,
    6,  6,  7,  7,  7,  7,  6,  6,
    6,  6,  7,  7,  7,  7,  6,  6,
    8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,
    9,  9,  9,  9,  9,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9
#elif (K_SIZE == 12)
// Table-12
     0,  1,  2,  3,  -3,  -2,  -1,  -0,
     4,  5,  6,  7,  -7,  -6,  -5,  -4,
     4,  5,  6,  7,  -7,  -6,  -5,  -4,
     8,  8,  9,  9,  -9,  -9,  -8,  -8,
     8,  8,  9,  9,  -9,  -9,  -8,  -8,
    10, 10, 11, 11, -11, -11, -10, -10,
    10, 10, 11, 11, -11, -11, -10, -10,
    10, 10, 11, 11, -11, -11, -10, -10
#elif (K_SIZE == 16)
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

using avx_register_type_16 = __m512i;
using avx_register_type_32 = __m512i;
#define avx_madd_epi16(a, b) (_mm512_madd_epi16(a, b))
#define avx_add_epi32(a, b)  (_mm512_add_epi32(a, b))
#define avx_sub_epi32(a, b)  (_mm512_sub_epi32(a, b))
#define avx_add_epi16(a, b)  (_mm512_add_epi16(a, b))
#define avx_sub_epi16(a, b)  (_mm512_sub_epi16(a, b))
#define avx_max_epi16(a, b)  (_mm512_max_epi16(a, b))
#define avx_min_epi16(a, b)  (_mm512_min_epi16(a, b))
#define avx_set1_epi16(a)    (_mm512_set1_epi16(a))

#elif defined(__AVX2__)

using avx_register_type_16 = __m256i;
using avx_register_type_32 = __m256i;
#define avx_madd_epi16(a, b) (_mm256_madd_epi16(a, b))
#define avx_add_epi32(a, b)  (_mm256_add_epi32(a, b))
#define avx_sub_epi32(a, b)  (_mm256_sub_epi32(a, b))
#define avx_add_epi16(a, b)  (_mm256_add_epi16(a, b))
#define avx_sub_epi16(a, b)  (_mm256_sub_epi16(a, b))
#define avx_max_epi16(a, b)  (_mm256_max_epi16(a, b))
#define avx_min_epi16(a, b)  (_mm256_min_epi16(a, b))
#define avx_set1_epi16(a)    (_mm256_set1_epi16(a))

#elif defined(__SSE2__)

using avx_register_type_16 = __m128i;
using avx_register_type_32 = __m128i;
#define avx_madd_epi16(a, b) (_mm_madd_epi16(a, b))
#define avx_add_epi32(a, b)  (_mm_add_epi32(a, b))
#define avx_sub_epi32(a, b)  (_mm_sub_epi32(a, b))
#define avx_add_epi16(a, b)  (_mm_add_epi16(a, b))
#define avx_sub_epi16(a, b)  (_mm_sub_epi16(a, b))
#define avx_max_epi16(a, b)  (_mm_max_epi16(a, b))
#define avx_min_epi16(a, b)  (_mm_min_epi16(a, b))
#define avx_set1_epi16(a)    (_mm_set1_epi16(a))

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

    this->_l1bias_avx = new (std::align_val_t(ALIGNMENT)) i16[L1_OUT_SIZE_P2];
    this->_l1data_avx = new (std::align_val_t(ALIGNMENT)) i16[IN_SIZE * L1_OUT_SIZE_P];
    this->_l2bias_avx = new (std::align_val_t(ALIGNMENT)) i16[OUT_SIZE];
    this->_l2data_avx = new (std::align_val_t(ALIGNMENT)) i16[HIDDEN_SIZE2 * OUT_SIZE];

    if (filename == "")
    {
#ifdef NN_FILE
        std::cout << "info string Loading model (incbin): " << NN_FILE << std::endl;
        int idx = 0;
        memcpy(this->_l1bias_avx, &gModelData[idx], L1_OUT_SIZE_P * sizeof(i16));
        idx += L1_OUT_SIZE_P * sizeof(i16);
        memcpy(this->_l1data_avx, &gModelData[idx], IN_SIZE * L1_OUT_SIZE_P * sizeof(i16));
        idx += IN_SIZE * L1_OUT_SIZE_P * sizeof(i16);
        memcpy(this->_l2bias_avx, &gModelData[idx], OUT_SIZE * sizeof(i16));
        idx += OUT_SIZE * sizeof(i16);
        memcpy(this->_l2data_avx, &gModelData[idx], HIDDEN_SIZE2 * OUT_SIZE * sizeof(i16));
        idx += HIDDEN_SIZE2 * OUT_SIZE * sizeof(i16);
        if (idx != gModelSize)
            std::cout << "if (idx != gModelSize) --- idx: " << idx << " --- gModelSize: " << gModelSize << std::endl;
#endif
    }
    else
    {
        std::cout << "info string Loading model: " << filename << std::endl;
        std::fstream file(filename, std::ios::in | std::ios::binary);
        file.read((char *)this->_l1bias_avx, L1_OUT_SIZE_P * sizeof(i16));
        file.read((char *)this->_l1data_avx, IN_SIZE * L1_OUT_SIZE_P * sizeof(i16));
        file.read((char *)this->_l2bias_avx, OUT_SIZE * sizeof(i16));
        file.read((char *)this->_l2data_avx, HIDDEN_SIZE2 * OUT_SIZE * sizeof(i16));
        file.close();
    }

    for (int i = 0; i < L1_OUT_SIZE_P; ++i)
        this->_l1bias_avx[L1_OUT_SIZE_P+i] = this->_l1bias_avx[i];

#else

    std::cout << "Neural Network disabled :(" << std::endl;

#endif
}

void Model::free()
{
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
}

Neural::~Neural()
{
}

void Neural::accum_init(int stack_pointer)
{
    Model &model = Model::instance();
    memcpy(this->_stack[stack_pointer]._layer1, model._l1bias_avx, L1_OUT_SIZE_P2 * sizeof(i16));
}

void Neural::accum_copy(int from, int to)
{
    memcpy(this->_stack[to]._layer1, this->_stack[from]._layer1, L1_OUT_SIZE_P2 * sizeof(i16));
}

void Neural::accum_piece_add(int stack_pointer, int wk, int bk, int color, int piece, int sq)
{
    int wk_side = ((wk & 7) > 3) ? 7 : 0;
    int bk_side = ((bk & 7) > 3) ? 7 : 0;

    int wk_idx = KING_TABLE[wk ^      wk_side];
    int bk_idx = KING_TABLE[bk ^ 56 ^ bk_side];

    int w_pos = wk_idx * P_SIZE +   (sq ^      wk_side) +   PIECE_TABLE[color][piece] *   64;
    int b_pos = bk_idx * P_SIZE +   (sq ^ 56 ^ bk_side) +   PIECE_TABLE[1-color][piece] * 64;

    Model &model = Model::instance();

#ifdef USE_INTRIN
    w_pos *= L1_OUT_SIZE_P/COUNT_16_BIT;
    b_pos *= L1_OUT_SIZE_P/COUNT_16_BIT;

    const auto l1data = (avx_register_type_16*) (model._l1data_avx);
    const auto sum = (avx_register_type_16*) (this->_stack[stack_pointer]._layer1);

    for (int i = 0; i < L1_OUT_SIZE_P/COUNT_16_BIT; ++i)
        sum[i] = avx_add_epi16(sum[i], l1data[w_pos+i]);
    for (int i = 0; i < L1_OUT_SIZE_P/COUNT_16_BIT; ++i)
        sum[i + L1_OUT_SIZE_P/COUNT_16_BIT] = avx_add_epi16(sum[i + L1_OUT_SIZE_P/COUNT_16_BIT], l1data[b_pos+i]);
#else
    w_pos *= L1_OUT_SIZE_P;
    b_pos *= L1_OUT_SIZE_P;

    for (int i = 0; i < L1_OUT_SIZE_P; i += UNROLL)
        for (int j = 0; j < UNROLL; ++j)
            this->_stack[stack_pointer]._layer1[i + j] += model._l1data_avx[w_pos + i + j];
    for (int i = 0; i < L1_OUT_SIZE_P; i += UNROLL)
        for (int j = 0; j < UNROLL; ++j)
            this->_stack[stack_pointer]._layer1[i + j + L1_OUT_SIZE_P] += model._l1data_avx[b_pos + i + j];
#endif
}

void Neural::accum_piece_remove(int stack_pointer, int wk, int bk, int color, int piece, int sq)
{
    int wk_side = ((wk & 7) > 3) ? 7 : 0;
    int bk_side = ((bk & 7) > 3) ? 7 : 0;

    int wk_idx = KING_TABLE[wk ^      wk_side];
    int bk_idx = KING_TABLE[bk ^ 56 ^ bk_side];

    int w_pos = wk_idx * P_SIZE +   (sq ^      wk_side) +   PIECE_TABLE[color][piece] *   64;
    int b_pos = bk_idx * P_SIZE +   (sq ^ 56 ^ bk_side) +   PIECE_TABLE[1-color][piece] * 64;

    Model &model = Model::instance();

#ifdef USE_INTRIN
    w_pos *= L1_OUT_SIZE_P/COUNT_16_BIT;
    b_pos *= L1_OUT_SIZE_P/COUNT_16_BIT;

    const auto l1data = (avx_register_type_16*) (model._l1data_avx);
    const auto sum = (avx_register_type_16*) (this->_stack[stack_pointer]._layer1);

    for (int i = 0; i < L1_OUT_SIZE_P/COUNT_16_BIT; ++i)
        sum[i] = avx_sub_epi16(sum[i], l1data[w_pos+i]);
    for (int i = 0; i < L1_OUT_SIZE_P/COUNT_16_BIT; ++i)
        sum[i + L1_OUT_SIZE_P/COUNT_16_BIT] = avx_sub_epi16(sum[i + L1_OUT_SIZE_P/COUNT_16_BIT], l1data[b_pos+i]);
#else
    w_pos *= L1_OUT_SIZE_P;
    b_pos *= L1_OUT_SIZE_P;

    for (int i = 0; i < L1_OUT_SIZE_P; i += UNROLL)
        for (int j = 0; j < UNROLL; ++j)
            this->_stack[stack_pointer]._layer1[i + j] -= model._l1data_avx[w_pos + i + j];
    for (int i = 0; i < L1_OUT_SIZE_P; i += UNROLL)
        for (int j = 0; j < UNROLL; ++j)
            this->_stack[stack_pointer]._layer1[i + j + L1_OUT_SIZE_P] -= model._l1data_avx[b_pos + i + j];
#endif
}

void Neural::accum_all_pieces(int stack_pointer, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq)
{
    accum_init(stack_pointer);

    int wk_pos = Bitboards::lsb(wk);
    int bk_pos = Bitboards::lsb(bk);

    // PAWNS
    while (wp != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 6, Bitboards::poplsb(wp));
    while (bp != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 6, Bitboards::poplsb(bp));

    // KNIGHTS
    while (wn != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 5, Bitboards::poplsb(wn));
    while (bn != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 5, Bitboards::poplsb(bn));

    // BISHOPS
    while (wb != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 4, Bitboards::poplsb(wb));
    while (bb != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 4, Bitboards::poplsb(bb));

    // ROOKS
    while (wr != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 3, Bitboards::poplsb(wr));
    while (br != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 3, Bitboards::poplsb(br));

    // QUEENS
    while (wq != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 2, Bitboards::poplsb(wq));
    while (bq != 0)
        accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 2, Bitboards::poplsb(bq));

    // KINGS
    accum_piece_add(stack_pointer, wk_pos, bk_pos, 0, 1, wk_pos);
    accum_piece_add(stack_pointer, wk_pos, bk_pos, 1, 1, bk_pos);
}

int Neural::accum_predict(int stack_pointer, int color, int stage)
{
    Model &model = Model::instance();

#ifdef USE_INTRIN
    int l2_idx = stage * HIDDEN_SIZE2/COUNT_16_BIT;

    const auto l2data = (avx_register_type_16*) (model._l2data_avx);
    const auto l1res = (avx_register_type_16*) (this->_stack[stack_pointer]._layer1);

    avx_register_type_32 sum {};
    avx_register_type_16 relu_max = avx_set1_epi16(0);
    avx_register_type_16 relu_min = avx_set1_epi16(QUANTIZATION_COEFF_L1);


    if (color)
    {
        for (int i = 0; i < HIDDEN_SIZE/COUNT_16_BIT; i++)
#ifdef CLIPPED_RELU
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_min_epi16(avx_max_epi16(l1res[L1_OUT_SIZE_P/COUNT_16_BIT + i], relu_max), relu_min), l2data[l2_idx + i]));
#else
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_max_epi16(l1res[L1_OUT_SIZE_P/COUNT_16_BIT + i], relu_max), l2data[l2_idx + i]));
#endif
        for (int i = 0; i < HIDDEN_SIZE/COUNT_16_BIT; i++)
#ifdef CLIPPED_RELU
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_min_epi16(avx_max_epi16(l1res[i], relu_max), relu_min), l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]));
#else
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_max_epi16(l1res[i], relu_max), l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]));
#endif
    }
    else
    {
        for (int i = 0; i < HIDDEN_SIZE/COUNT_16_BIT; i++)
#ifdef CLIPPED_RELU
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_min_epi16(avx_max_epi16(l1res[i], relu_max), relu_min), l2data[l2_idx + i]));
#else
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_max_epi16(l1res[i], relu_max), l2data[l2_idx + i]));
#endif
        for (int i = 0; i < HIDDEN_SIZE/COUNT_16_BIT; i++)
#ifdef CLIPPED_RELU
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_min_epi16(avx_max_epi16(l1res[L1_OUT_SIZE_P/COUNT_16_BIT + i], relu_max), relu_min), l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]));
#else
            sum = avx_add_epi32(sum, avx_madd_epi16(avx_max_epi16(l1res[L1_OUT_SIZE_P/COUNT_16_BIT + i], relu_max), l2data[l2_idx + HIDDEN_SIZE/COUNT_16_BIT + i]));
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
#else
    int l2_idx = stage * HIDDEN_SIZE2;

    int res_i = model._l2bias_avx[stage];
    if (color)
    {
        // TODO: ADD CLIPPED_RELU!!!!

        for (int i = 0; i < HIDDEN_SIZE; i += UNROLL)
            for (int j = 0; j < UNROLL; ++j)
                res_i += std::max(this->_stack[stack_pointer]._layer1[i + j + L1_OUT_SIZE_P], (short)0) * model._l2data_avx[l2_idx + i + j] ;
        for (int i = 0; i < HIDDEN_SIZE; i += UNROLL)
            for (int j = 0; j < UNROLL; ++j)
                res_i += std::max(this->_stack[stack_pointer]._layer1[i + j], (short)0) * model._l2data_avx[l2_idx + i + j + HIDDEN_SIZE];
    }
    else
    {
        for (int i = 0; i < HIDDEN_SIZE; i += UNROLL)
            for (int j = 0; j < UNROLL; ++j)
                res_i += std::max(this->_stack[stack_pointer]._layer1[i + j], (short)0) * model._l2data_avx[l2_idx + i + j] ;
        for (int i = 0; i < HIDDEN_SIZE; i += UNROLL)
            for (int j = 0; j < UNROLL; ++j)
                res_i += std::max(this->_stack[stack_pointer]._layer1[i + j + L1_OUT_SIZE_P], (short)0) * model._l2data_avx[l2_idx + i + j + HIDDEN_SIZE];
    }

    float res = res_i;
#endif
    res *= EVAL_DIVIDER / (QUANTIZATION_COEFF_L1*QUANTIZATION_COEFF_L2);

#ifdef PADDING
    int psqt = (this->_stack[stack_pointer]._layer1[HIDDEN_SIZE + stage] - this->_stack[stack_pointer]._layer1[L1_OUT_SIZE_P + HIDDEN_SIZE + stage]) / 2;
    if (color)
        psqt = -psqt;
    res += psqt;
#endif

    if (color)
        res = -res;
    return static_cast<int>(res);
}

int Neural::predict_i(int color, u64 wk, u64 bk, u64 wp, u64 bp, u64 wn, u64 bn, u64 wb, u64 bb, u64 wr, u64 br, u64 wq, u64 bq)
{
    this->accum_all_pieces(0, wk, bk, wp, bp, wn, bn, wb, bb, wr, br, wq, bq);
    int stg = Neural::stage(Bitboards::bits_count(wk | bk | wp | bp | wn | bn | wb | bb | wr | br | wq | bq), (wq | bq) != 0);
    return accum_predict(0, color, stg);
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
