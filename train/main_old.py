import math
import os
import json
from os import listdir
from os.path import isfile, join
import tensorflow as tf
from keras.models import Model, load_model
from keras.layers import Input, Concatenate, Dense
import numpy as np
from keras import backend as K
from keras.callbacks import ModelCheckpoint, LearningRateScheduler
from batch_loader import load, convert

# os.environ['TF_CONFIG'] = json.dumps({
#     'cluster': {
#         'worker': ["192.168.0.64:12345", "192.168.0.105:12345"]
#     },
#     'task': {'type': 'worker', 'index': 0}
# })

K_SIZE = 10
P_SIZE = 768
INPUT_SIZE = K_SIZE * P_SIZE
HIDDEN_LAYER_SIZE = 512
BATCH_SIZE = 10000
EPOCHS = 25
LAMBDA = 10
EVAL_DIVIDER = 400
INITIAL_EPOCH = 0
MODEL_FILE = ""
NEURAL_FILE = "neural.txt"
NN_FILE = "nn_0.8ct_l" + str(LAMBDA) + "_e" + str(EPOCHS) + ".nn"
QUANTIZATION_COEFF_L1 = 256
QUANTIZATION_COEFF_L2 = 512
START_LR = 0.0011
LR_GAMMA = 0.9
ALL_FILES = [f for f in listdir("data") if isfile(join("data", f))]
FILE_SIZE = 10000000
TRAIN_FILES = []
TEST_FILES = []

FILES_COUNT = 0
for file in ALL_FILES:
    if FILES_COUNT < 1:
        TEST_FILES.append(join("data", file))
    else:
        TRAIN_FILES.append(join("data", file))
    FILES_COUNT += 1
ITERATIONS = FILE_SIZE * len(TRAIN_FILES) // BATCH_SIZE
ITERATIONS_TEST = FILE_SIZE * len(TEST_FILES) // BATCH_SIZE


def clipped_relu(x):
    return K.relu(x, max_value=1.0)


def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))


def lr_scheduler(epoch, lr):
    new_lr = START_LR * math.pow(LR_GAMMA, epoch)
    print("lr_scheduler -- epoch: " + str(epoch) + " -- lr: " + str(lr) + " -- new_lr: " + str(new_lr))
    return new_lr


def chess_generator(files, file_size, batch_size, is_exit):
    data_size = len(files) * file_size
    iterations = data_size // batch_size    # total in all files
    batches = file_size // batch_size       # in each file

    count = 0
    while True:
        for current_files in files:
            data = np.load(current_files)[0:file_size]
            data_counter = np.zeros(1, dtype=np.int32)

            while True:
                _in1 = np.zeros((batch_size, INPUT_SIZE), dtype=np.byte)
                _in2 = np.zeros((batch_size, INPUT_SIZE), dtype=np.byte)
                _out = np.zeros(batch_size, dtype=np.single)

                load(_in1, _in2, _out, data, data_counter, batch_size, LAMBDA, EVAL_DIVIDER)

                # print("yield")
                yield [_in1, _in2], _out

                count += 1
                if (count % batches) == 0:
                    if count == iterations:
                        # print(id + "  count == iterations!")
                        count = 0
                        if is_exit:
                            return
                    del data
                    break


# strategy = tf.distribute.experimental.MultiWorkerMirroredStrategy()
# with strategy.scope():
if os.path.exists(MODEL_FILE):
    print("Loading model from file: " + MODEL_FILE)
    model = load_model(MODEL_FILE, custom_objects={"clipped_relu": clipped_relu})
else:
    print("New model")
    input1 = Input(INPUT_SIZE, dtype=np.byte)
    input2 = Input(INPUT_SIZE, dtype=np.byte)

    layer1 = Dense(HIDDEN_LAYER_SIZE, activation="relu")    # clipped_relu  "relu"
    layer1_concat = Concatenate()([layer1(input1), layer1(input2)])

    layer2 = Dense(1, activation="sigmoid")(layer1_concat)

    model = Model(inputs=[input1, input2], outputs=layer2)

    model.compile(loss='mean_squared_error', optimizer="adam", metrics=["accuracy"])    # SGD   adam

if not(INITIAL_EPOCH == 0 and MODEL_FILE != ""):
    print(model.summary())

    filepath = "model_l" + str(LAMBDA) + "_{epoch:02d}_{loss:.6f}_{val_loss:.6f}.h5"
    checkpoint = ModelCheckpoint(filepath, monitor='loss')
    scheduler = LearningRateScheduler(lr_scheduler)
    callbacks_list = [checkpoint, scheduler]

    model.fit(x=chess_generator(TRAIN_FILES, FILE_SIZE, BATCH_SIZE, False),
              epochs=EPOCHS,
              initial_epoch=INITIAL_EPOCH,
              steps_per_epoch=ITERATIONS,
              validation_data=chess_generator(TEST_FILES, FILE_SIZE, BATCH_SIZE, False),
              validation_steps=ITERATIONS_TEST,
              callbacks=callbacks_list,
              max_queue_size=10,
              workers=1,
              use_multiprocessing=False)

weights_all = []
weights_all.append(model.layers[2].weights[0].numpy().tolist())
weights_all.append(model.layers[2].weights[1].numpy().tolist())
weights_all.append(model.layers[4].weights[0].numpy().tolist())
weights_all.append(model.layers[4].weights[1].numpy().tolist())

with open(NEURAL_FILE, 'w') as file:
    file.write(str(INPUT_SIZE) + "\n")
    file.write(str(HIDDEN_LAYER_SIZE) + "\n")
    for w in weights_all[1]:
        file.write(str(w) + "\n")
    for ws in weights_all[0]:
        for w in ws:
            file.write(str(w) + "\n")
    for w in weights_all[3]:
        file.write(str(w) + "\n")
    for ws in weights_all[2]:
        for w in ws:
            file.write(str(w) + "\n")

convert(NEURAL_FILE, NN_FILE, QUANTIZATION_COEFF_L1, QUANTIZATION_COEFF_L2)
