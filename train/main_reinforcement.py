import subprocess
import json
from math import sqrt
import torch
from os.path import exists
import ranger
from train import train, set_lr
from utils import Files
from model import Net
from batch_loader import chess_generator

CONFIG_FILE = "reinforcement.json"

DATA_DIR = "data"
RESULT_DIR = "res/r0"

URALOCHKA = "Uralochka3-r"

THREADS = 16
BOOK = ""

START_GEN_FILES = 10

ITERATIONS = 1000

BATCH_SIZE = 10000
FILE_SIZE = 2000000

START_LR = 0.001
LR_COEFF = 0.999

LAMBDA = 500
EVAL_DIVIDER = 400

NN_TEMPLATE = RESULT_DIR + "/rl_0.0a_i{}_l{}.nn"
MODEL_TEMPLATE = RESULT_DIR + "/model_rl_0.0a_{:04d}_l{:02d}_{:.6f}.pt"


with open(CONFIG_FILE) as f:
    config = json.load(f)

use_cuda = torch.cuda.is_available()
print("CUDA: " + str(use_cuda))

device = torch.device("cuda")

model = Net(EVAL_DIVIDER)
model = model.to(device)

optimizer = ranger.Ranger(model.parameters(), lr=START_LR, betas=(.9, 0.999), eps=1.0e-7, gc_loc=False, use_gc=False)

model_file = config["model_file"]
if exists(model_file):
    print("\nLoading model from file: " + model_file)
    model.load_model(model_file, optimizer)

nn_file = config["nn_file"]
if not exists(nn_file):
    config["nn_file"] = NN_TEMPLATE.format(0, LAMBDA)
    model.save_model(None, config["nn_file"], None, 0)

# subprocess.run([URALOCHKA, "nn", config["nn_file"], "bench"])

iteration = config["iteration"]

if iteration == 0:
    config["lr"] = START_LR

for itr in range(iteration, ITERATIONS):
    iteration += 1
    print("\n\nIteration " + str(iteration))

    keep_files = START_GEN_FILES + int(sqrt(iteration))
    gen_files = int(sqrt(keep_files))
    if iteration == 1:
        gen_files = START_GEN_FILES

    print("keep_files =", keep_files)
    print("gen_files =", gen_files, "\n")

    subprocess.run([URALOCHKA,
                    "nn", config["nn_file"],
                    "dg", str(THREADS), str(gen_files), str(config["file_idx"]), BOOK])

    set_lr(optimizer, config["lr"])

    train_files = Files(DATA_DIR, shuffle_first=False, shuffle_iter=False, keep_last=keep_files)
    print(train_files.files)

    train_loss = train(model, optimizer,
                       chess_generator(train_files, 0, FILE_SIZE, BATCH_SIZE, LAMBDA,
                                       1000, BATCH_SIZE, EVAL_DIVIDER, device),
                       iteration, ITERATIONS, train_files.size() * FILE_SIZE // BATCH_SIZE)

    config["iteration"] = iteration
    config["file_idx"] += gen_files
    config["model_file"] = MODEL_TEMPLATE.format(iteration, LAMBDA, train_loss)
    config["nn_file"] = NN_TEMPLATE.format(iteration, LAMBDA)
    config["lr"] *= LR_COEFF

    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, ensure_ascii=False, indent=4)

    model.save_model(config["model_file"],
                     config["nn_file"],
                     optimizer,
                     iteration)
