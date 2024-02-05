#include <cmath>
#include <thread>
#include <string>
#include <vector>
#include <thread>
#include <memory.h>

#include <cnpy.h>

#include "ndarray.h"

#define K_SIZE      (16)
#define P_SIZE      (768)
#define KP_SIZE     (K_SIZE * P_SIZE)
#define FILE_SIZE   (10000000)
#define EVAL_COEFF  (0.5f)

float sigmoid(float data)
{
    return 1.0f / (1.0f + std::exp(-data));
}

static const int KING_TABLE[64] =
{
// Table-10
//    0,  1,  2,  3,  3,  2,  1,  0,
//    4,  4,  5,  5,  5,  5,  4,  4,
//    6,  6,  7,  7,  7,  7,  6,  6,
//    6,  6,  7,  7,  7,  7,  6,  6,
//    8,  8,  8,  8,  8,  8,  8,  8,
//    8,  8,  8,  8,  8,  8,  8,  8,
//    9,  9,  9,  9,  9,  9,  9,  9,
//    9,  9,  9,  9,  9,  9,  9,  9
// Table-12
//     0,  1,  2,  3,  3,  2,  1,  0,
//     4,  5,  6,  7,  7,  6,  5,  4,
//     4,  5,  6,  7,  7,  6,  5,  4,
//     8,  8,  9,  9,  9,  9,  8,  8,
//     8,  8,  9,  9,  9,  9,  8,  8,
//    10, 10, 11, 11, 11, 11, 10, 10,
//    10, 10, 11, 11, 11, 11, 10, 10,
//    10, 10, 11, 11, 11, 11, 10, 10
// Table-16 (Koivisto)
     0,  1,  2,  3,  3,  2,  1,  0,
     4,  5,  6,  7,  7,  6,  5,  4,
     8,  9, 10, 11, 11, 10,  9,  8,
     8,  9, 10, 11, 11, 10,  9,  8,
    12, 12, 13, 13, 13, 13, 12, 12,
    12, 12, 13, 13, 13, 13, 12, 12,
    14, 14, 15, 15, 15, 15, 14, 14,
    14, 14, 15, 15, 15, 15, 14, 14
// Table-24
//     0,  1,  2,  3,  3,  2,  1,  0,
//     4,  5,  6,  7,  7,  6,  5,  4,
//     8,  9, 10, 11, 11, 10,  9,  8,
//    12, 13, 14, 15, 15, 14, 13, 12,
//    16, 17, 18, 19, 19, 18, 17, 16,
//    16, 17, 18, 19, 19, 18, 17, 16,
//    20, 21, 22, 23, 23, 22, 21, 20,
//    20, 21, 22, 23, 23, 22, 21, 20
// Table-32
//     0,  1,  2,  3,  3,  2,  1,  0,
//     4,  5,  6,  7,  7,  6,  5,  4,
//     8,  9, 10, 11, 11, 10,  9,  8,
//    12, 13, 14, 15, 15, 14, 13, 12,
//    16, 17, 18, 19, 19, 18, 17, 16,
//    20, 21, 22, 23, 23, 22, 21, 20,
//    24, 25, 26, 27, 27, 26, 25, 24,
//    28, 29, 30, 31, 31, 30, 29, 28
};

typedef unsigned char u8;

int load(std::string filename,
         numpyArray<int> _in1,
         numpyArray<int> _in2,
         numpyArray<long long> _stg,
         numpyArray<float> _out,
         numpyArray<int> _bat,
         int batch_size,
         int wdl_percent,
         int draws_percent_i,
         int draws_percent_n,
         int eval_divider)
{
    Ndarray<int, 2> in1(_in1);
    Ndarray<int, 2> in2(_in2);
    Ndarray<long long, 2> stg(_stg);
    Ndarray<float, 1> out(_out);
    Ndarray<int, 2> bat(_bat);

    auto arr = cnpy::npy_load(filename);
    auto data = arr.as_vec<u8>();

    const int batches = FILE_SIZE / batch_size;

    const float lambda = static_cast<float>(wdl_percent) / 1000.0f;
    const float draws_percent = static_cast<float>(draws_percent_i);

    int counter = 0;
    for (int btch = 0; btch < batches; btch++)
    {
//        for (int i = 0; i < 12; ++i)    // Make const!
//            bat[btch][2 + i] = 0;
        int real_idx = 0;
        int draws = 0;
        int start_idx = batch_size * btch;
        for (int i = 0; i < batch_size; ++i)
        {
            int res1 = data[counter++];
            int res2 = data[counter++];
            int eval1 = data[counter++];
            int eval2 = data[counter++];
            int flags = data[counter++];

            int game_res = flags & 3;
            int color = (flags >> 2) & 1;
            int res_sign = (flags >> 3) & 1;
            int eval_sign = (flags >> 4) & 1;

            if (game_res == 1)   // draw
                draws++;

            float percent = 1000.0f * draws / (real_idx + 1.0f);
            bool add_position = (i < draws_percent_n || percent <= draws_percent);

            int res = ((res1 * 256) + res2) * (res_sign ? -1 : 1);
            if (res > 3000)
                res = 3000 + (res - 3000) / 5;
            else if (res < -3000)
                res = -3000 + (res + 3000) / 5;

            int eval = ((eval1 * 256) + eval2) * (eval_sign ? -1 : 1);
            if (eval > 3000)
                eval = 3000 + (eval - 3000) / 5;
            else if (eval < -3000)
                eval = -3000 + (eval + 3000) / 5;

//            res = std::max(res, eval-5);
//            res = std::min(res, eval+5);
//            float value = static_cast<float>(res);

            float value = static_cast<float>(res) * (1.0f - EVAL_COEFF) + static_cast<float>(eval) * EVAL_COEFF;

            value = sigmoid(value / static_cast<float>(eval_divider));
            float wdl = static_cast<float>(game_res) / 2.0f;

            float use_lambda = lambda;
            if (game_res == 1)
                use_lambda *= 1.5f;
//            if (abs(res) > 1500)
//                use_lambda /= 2.0f;
//            if (abs(res) > 2500)
//                use_lambda /= 2.0f;

            out[start_idx+real_idx] = wdl * use_lambda + value * (1.0f - use_lambda);

            int pieces_cnt = -1;
            int idx_tmp = counter + 2;
            for (int piece = 0; piece < 10; ++piece)
            {
                int num = data[idx_tmp++];
                pieces_cnt += num;
                idx_tmp += num;
            }
            if (pieces_cnt < 0)
                pieces_cnt = 0;
            int stage_idx = pieces_cnt / 5;    // 0..5
            stg[start_idx+real_idx][0] = stage_idx;

            int wk_sq = data[counter++];
            int bk_sq = data[counter++];

            int wk = KING_TABLE[wk_sq] * P_SIZE;
            int bk = KING_TABLE[bk_sq ^ 56] * P_SIZE;

            int wk_side = ((wk_sq & 7) > 3) ? 7 : 0;
            int bk_side = ((bk_sq & 7) > 3) ? 7 : 0;

            if (add_position)
            {
//                bat[btch][2 + stage_idx]++;

                int wk1 = 0;
                int bk1 = 0;
                int wk2 = 0;
                int bk2 = 0;

                if (color)  // black
                {
                    out[start_idx+real_idx] = 1.0f - out[start_idx+real_idx];

                    wk2 = wk + 10*64 + (wk_sq ^ wk_side);
                    bk2 = wk + 11*64 + (bk_sq ^ wk_side);

                    wk1 = bk + 10*64 + (bk_sq ^ 56 ^ bk_side);
                    bk1 = bk + 11*64 + (wk_sq ^ 56 ^ bk_side);
                }
                else        // white
                {
                    wk1 = wk + 10*64 + (wk_sq ^ wk_side);
                    bk1 = wk + 11*64 + (bk_sq ^ wk_side);

                    wk2 = bk + 10*64 + (bk_sq ^ 56 ^ bk_side);
                    bk2 = bk + 11*64 + (wk_sq ^ 56 ^ bk_side);
                }

                in1[start_idx+real_idx][0] = wk1;
                in1[start_idx+real_idx][1] = bk1;

                in2[start_idx+real_idx][0] = wk2;
                in2[start_idx+real_idx][1] = bk2;

                int piece_number = 2;
                for (int piece = 0; piece < 10; ++piece)
                {
                    int num = data[counter++];
                    int piece_w = wk + piece * 64;
                    int piece_b = bk + (piece < 5 ? piece + 5 : piece - 5) * 64;

                    for (int j = 0; j < num; ++j)
                    {
                        int sq = data[counter++];

                        if (color)  // black
                        {
                            in2[start_idx+real_idx][piece_number] = piece_w + (sq ^ wk_side);
                            in1[start_idx+real_idx][piece_number] = piece_b + (sq ^ 56 ^ bk_side);
                        }
                        else        // white
                        {
                            in1[start_idx+real_idx][piece_number] = piece_w + (sq ^ wk_side);
                            in2[start_idx+real_idx][piece_number] = piece_b + (sq ^ 56 ^ bk_side);
                        }
                        piece_number++;
                    }
                }

                for (int j = piece_number; j < 32; ++j)
                {
                    in1[start_idx+real_idx][piece_number] = -1;
                    in2[start_idx+real_idx][piece_number] = -1;
                }
                real_idx++;
            }
            else
            {
                counter = idx_tmp;
                draws--;
            }
        }

        bat[btch][0] = real_idx;
        bat[btch][1] = draws;
    }

    return 0;
}

std::thread* thread = nullptr;
numpyArray<int> in1;
numpyArray<int> in2;
numpyArray<long long> stg;
numpyArray<float> out;
numpyArray<int> bat;
int in1_size = 0;
int in2_size = 0;
int stg_size = 0;
int out_size = 0;
int bat_size = 0;

extern "C" {

int loader_load(char* filename,
                int batch_size,
                int wdl_percent,
                int draws_percent_i,
                int draws_percent_n,
                int eval_divider)
{
    if (thread != nullptr)
        return 1;
    thread = new std::thread(load, std::string(filename), in1, in2, stg, out, bat, batch_size, wdl_percent, draws_percent_i, draws_percent_n, eval_divider);
    return 0;
}

int loader_get(numpyArray<int> _in1,
               numpyArray<int> _in2,
               numpyArray<long long> _stg,
               numpyArray<float> _out,
               numpyArray<int> _bat)
{
    if (thread == nullptr)
        return 1;
    thread->join();
    delete thread;
    thread = nullptr;

    memcpy(_in1.data, in1.data, in1_size * sizeof(int));
    memcpy(_in2.data, in2.data, in2_size * sizeof(int));
    memcpy(_stg.data, stg.data, stg_size * sizeof(long long));
    memcpy(_out.data, out.data, out_size * sizeof(float));
    memcpy(_bat.data, bat.data, bat_size * sizeof(int));

    return 0;
}

int loader_init(numpyArray<int> _in1,
                numpyArray<int> _in2,
                numpyArray<long long> _stg,
                numpyArray<float> _out,
                numpyArray<int> _bat,
                int batch_size)
{
    in1_size = _in1.strides[0] * FILE_SIZE;
    in2_size = _in2.strides[0] * FILE_SIZE;
    stg_size = _stg.strides[0] * FILE_SIZE;
    out_size = _out.strides[0] * FILE_SIZE;
    bat_size = _bat.strides[0] * FILE_SIZE / batch_size;

    in1.data = new int[in1_size];
    in1.shape = new long[2];
    in1.strides = new long[2];

    in2.data = new int[in2_size];
    in2.shape = new long[2];
    in2.strides = new long[2];

    stg.data = new long long[stg_size];
    stg.shape = new long[2];
    stg.strides = new long[2];

    out.data = new float[out_size];
    out.shape = new long[1];
    out.strides = new long[1];

    bat.data = new int[bat_size];
    bat.shape = new long[2];
    bat.strides = new long[2];

    in1.shape[0] = _in1.shape[0];
    in1.shape[1] = _in1.shape[1];
    in1.strides[0] = _in1.strides[0];
    in1.strides[1] = _in1.strides[1];

    in2.shape[0] = _in2.shape[0];
    in2.shape[1] = _in2.shape[1];
    in2.strides[0] = _in2.strides[0];
    in2.strides[1] = _in2.strides[1];

    stg.shape[0] = _stg.shape[0];
    stg.shape[1] = _stg.shape[1];
    stg.strides[0] = _stg.strides[0];
    stg.strides[1] = _stg.strides[1];

    out.shape[0] = _out.shape[0];
    out.strides[0] = _out.strides[0];

    bat.shape[0] = _bat.shape[0];
    bat.shape[1] = _bat.shape[1];
    bat.strides[0] = _bat.strides[0];
    bat.strides[1] = _bat.strides[1];

    return 0;
}

int loader_free()
{
    delete []in1.data;
    delete []in1.shape;
    delete []in1.strides;

    delete []in2.data;
    delete []in2.shape;
    delete []in2.strides;

    delete []stg.data;
    delete []stg.shape;
    delete []stg.strides;

    delete []out.data;
    delete []out.shape;
    delete []out.strides;

    delete []bat.data;
    delete []bat.shape;
    delete []bat.strides;

    return 0;
}


} // end extern "C"
