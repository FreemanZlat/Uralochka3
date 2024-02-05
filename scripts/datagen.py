import subprocess
import json

CONFIG_FILE = "datagen.json"

URALOCHKA = "./Uralochka3-gen"
NN_FILE = "../nn_1.0a_e90_l200_d380.nn"
GEN_FILES = 1
THREADS = 8
HASH1 = 4096
HASH2 = 4096
BOOK = ""

ITERATIONS = 1000

with open(CONFIG_FILE) as f:
    config = json.load(f)

subprocess.run([URALOCHKA, "nn", NN_FILE, "bench"])

iteration = config["iteration"]

for itr in range(iteration, ITERATIONS):
    iteration += 1
    print("\n\nIteration " + str(iteration))

    subprocess.run([URALOCHKA,
                    "nn", NN_FILE,
                    "dg", str(THREADS), str(GEN_FILES), str(config["file_idx"]), str(HASH1), str(HASH2), BOOK])

    config["iteration"] = iteration
    config["file_idx"] += GEN_FILES

    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, ensure_ascii=False, indent=4)
