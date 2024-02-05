import torch
from os.path import exists
import ranger
from train import test, train, set_lr
from utils import telegram_bot_sendtext, Files
from model import Net
from batch_loader import chess_generator

BATCH_SIZE = 10000
FILE_SIZE = 10000000

START_LR = 0.001
EPOCHS = 500
EPOCH_SIZE = 40 * FILE_SIZE

DRAWS_PERCENT = 400
DRAWS_PERCENT_N = 1500

EVAL_DIVIDER = 500

MODEL_FILE = "res/1.pt"
ONLY_SAVE = False
NN_TEMPLATE = "res/nn_1.2c_e{}_l{}_d{}.nn"
MODEL_TEMPLATE = "res/model_{:02d}_l{:02d}_d{:02d}_{:.6f}_{:.6f}.pt"

TRAIN_DIR = "/home/freeman/mnt/data_train"
TEST_DIR = "/home/freeman/mnt/data_test"
# TRAIN_DIR = "data_train"
# TEST_DIR = "data_test"

params = []

lmbd_start = 50
lmbd_step = (300 - 50) / (EPOCHS - 1)
start_lr = START_LR
for epoch in range(EPOCHS):
    params.append({ "lr": start_lr, "lmbd": round(lmbd_start) })
    start_lr *= 0.993
    lmbd_start += lmbd_step

# 01-20
# start_lmbd = 100
# lmbd_step = 0
# start_lr = START_LR
# for i in range(20):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.88
#     start_lmbd += lmbd_step
# # 21-30
# start_lr = START_LR*0.5
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.82
#     start_lmbd += lmbd_step
# # 31-40
# start_lr = START_LR*0.35
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step
# # 41-50
# start_lmbd += 50
# start_lr = START_LR*0.35
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step
# # 51-60
# start_lmbd += 50
# start_lr = START_LR*0.25
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step
# # 61-70
# start_lmbd += 50
# start_lr = START_LR*0.25
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step
# # 71-80
# start_lr = START_LR*0.25
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step

# # 81-90
# start_lmbd -= 50
# start_lr = START_LR*0.35
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step
# # 91-100
# start_lr = START_LR*0.25
# for i in range(10):
#     params.append({ "lr": start_lr, "lmbd": round(start_lmbd) })
#     start_lr *= 0.84
#     start_lmbd += lmbd_step


# https://towardsdatascience.com/understanding-learning-rates-and-how-it-improves-performance-in-deep-learning-d0d4059c1c10

for epoch in range(EPOCHS):
    print("Epoch: {} -- LR: {:.6f} -- LMBD: {}".format(epoch+1, params[epoch]["lr"], params[epoch]["lmbd"]))

use_cuda = torch.cuda.is_available()
print("CUDA: " + str(use_cuda))

device = torch.device("cuda")

train_files = Files(TRAIN_DIR)
test_files = Files(TEST_DIR, shuffle_first=False, shuffle_iter=False)
print(test_files.files)
print("Train files: {}".format(len(train_files.files)))

initial_epoch = 1

model = Net(EVAL_DIVIDER)
model = model.to(device)

# model_old = NetOld()
# checkpoint = torch.load("model_90_l200_d380_0.008247_0.008338.pt")
# model_old.load_state_dict(checkpoint['model'])

optimizer = ranger.Ranger(model.parameters(), lr=START_LR, betas=(.9, 0.999), eps=1.0e-7, gc_loc=False, use_gc=False)
# optimizer = optim.Adam(model.parameters(), lr=START_LR)

# convert(model_old, model)
# model.save_model(MODEL_TEMPLATE, NN_TEMPLATE, optimizer, 90, 200, DRAWS_PERCENT, 0, 0)


if exists(MODEL_FILE):
    print("\nLoading model from file: " + MODEL_FILE)
    initial_epoch = model.load_model(MODEL_FILE, optimizer)

if not ONLY_SAVE:
    telegram_bot_sendtext("Start. Initial epoch: " + str(initial_epoch))

    test_loss = test(model,
                     chess_generator(test_files, 0, FILE_SIZE, BATCH_SIZE, params[initial_epoch-1]["lmbd"],
                                     DRAWS_PERCENT, DRAWS_PERCENT_N, EVAL_DIVIDER, device),
                     test_files.size() * FILE_SIZE // BATCH_SIZE)

    for epoch in range(initial_epoch, EPOCHS+1):
        lmbd = params[epoch-1]["lmbd"]
        set_lr(optimizer, params[epoch-1]["lr"])
        lr = optimizer.param_groups[0]["lr"]
        print("\nEpoch: {}/{} -- LR: {:.6f} -- Lambda: {}".format(epoch, EPOCHS, lr, lmbd))

        train_loss = train(model, optimizer,
                           chess_generator(train_files, EPOCH_SIZE // FILE_SIZE, FILE_SIZE, BATCH_SIZE, lmbd,
                                           DRAWS_PERCENT, DRAWS_PERCENT_N, EVAL_DIVIDER, device),
                           epoch, EPOCHS, EPOCH_SIZE // BATCH_SIZE)

        if (epoch % 5) == 0:
            test_loss = test(model,
                             chess_generator(test_files, 0, FILE_SIZE, BATCH_SIZE, lmbd,
                                             DRAWS_PERCENT, DRAWS_PERCENT_N, EVAL_DIVIDER, device),
                             test_files.size() * FILE_SIZE // BATCH_SIZE)

            model.save_model(MODEL_TEMPLATE.format(epoch, lmbd, DRAWS_PERCENT, train_loss, test_loss),
                             NN_TEMPLATE.format(epoch, lmbd, DRAWS_PERCENT),
                             optimizer, epoch)

            telegram_bot_sendtext("Epoch {} done.\nTrain loss: {:.6f}\nTest loss: {:.6f}\nLR: {:.6f}\nLambda: {}".format(
                epoch, train_loss, test_loss, lr, lmbd))

else:
    model.save_model(None,
                     NN_TEMPLATE.format(EPOCHS, params[initial_epoch-1]["lmbd"], DRAWS_PERCENT),
                     None, EPOCHS)
