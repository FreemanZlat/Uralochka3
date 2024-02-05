import numpy as np
import torch
from numpy.ctypeslib import load_library
from numpyctypes import c_ndarray
import ctypes as C


INPUT_SIZE_MAX = 32

mylib = load_library('liburalochka3_batch', '.')


def loader_load(file, batch_size, wdl_pct, draws_pct, draws_pct_n, eval_div):
    b_file = file.encode('utf-8')
    return mylib.loader_load(C.c_char_p(b_file),
                             C.c_int(batch_size),
                             C.c_int(wdl_pct),
                             C.c_int(draws_pct),
                             C.c_int(draws_pct_n),
                             C.c_int(eval_div))


def loader_get(_in1, _in2, _stg, _out, _bat):
    return mylib.loader_get(c_ndarray(_in1),
                            c_ndarray(_in2),
                            c_ndarray(_stg),
                            c_ndarray(_out),
                            c_ndarray(_bat))


def loader_init(_in1, _in2, _stg, _out, _bat, batch_size):
    return mylib.loader_init(c_ndarray(_in1),
                             c_ndarray(_in2),
                             c_ndarray(_stg),
                             c_ndarray(_out),
                             c_ndarray(_bat),
                             C.c_int(batch_size))


def loader_free():
    return mylib.loader_free()


def chess_generator(files, files_count, file_size, batch_size, lmbd, draws_p, draws_n, eval_divider, device):
    batches = file_size // batch_size       # in each file

    if files_count == 0:
        files_count = files.size()

    _val = torch.from_numpy(np.full((batch_size, INPUT_SIZE_MAX), 1.0, dtype=np.single)).to(device)

    _in1 = np.full((file_size, INPUT_SIZE_MAX), -1, dtype=np.int32)
    _in2 = np.full((file_size, INPUT_SIZE_MAX), -1, dtype=np.int32)
    _stg = np.zeros((file_size, 1), dtype=np.int64)
    _out = np.zeros(file_size, dtype=np.single)
    _bat = np.zeros((batches, 2), dtype=np.int32)  # + 12 Make const

    loader_init(_in1, _in2, _stg, _out, _bat, batch_size)

    loader_load(files.get_file(), batch_size, lmbd, draws_p, draws_n, eval_divider)
    for file in range(files_count):
        loader_get(_in1, _in2, _stg, _out, _bat)
        if file < files_count - 1:
            loader_load(files.get_file(), batch_size, lmbd, draws_p, draws_n, eval_divider)

        # cnt = np.zeros(12, dtype=np.single)     # Make const

        real_count = 0
        draws_count = 0
        for btch in range(batches):
            real_batches = _bat[btch, 0]
            btch_from = btch * batch_size
            btch_to = btch_from + real_batches

            # for i in range(12):
            #     cnt[i] += _bat[btch, 2+i]

            in1 = torch.from_numpy(_in1[btch_from:btch_to, 0:INPUT_SIZE_MAX]).to(device, non_blocking=True)
            in2 = torch.from_numpy(_in2[btch_from:btch_to, 0:INPUT_SIZE_MAX]).to(device, non_blocking=True)
            stg = torch.from_numpy(_stg[btch_from:btch_to, 0:1]).to(device, non_blocking=True)
            out = torch.from_numpy(_out[btch_from:btch_to]).to(device, non_blocking=True)
            val = _val[0:real_batches, 0:INPUT_SIZE_MAX]

            real_count += real_batches
            draws_count += _bat[btch, 1]

            yield [in1, in2, val, stg], out

        # stgs = "\n"
        # for i in range(3):      # Make const
        #     stgs += str(i) + ": " + str(round(cnt[i] * 1000.0 / real_count) / 10.0) + "% --- "
        # print(stgs)

        # print("Draws: " + str(100.0 * draws_count / real_count) +
        #       "% (" + str(draws_count) + "/" + str(real_count) + ")\n")

    loader_free()
