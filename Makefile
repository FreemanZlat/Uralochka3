# Makefile only for OpenBench :)

CXX ?= clang++
EXE ?= Uralochka3
EVALFILE ?= nn/nn_1.3db_e500_l400_d400.nn

SRC = src/*.cpp src/fathom/tbprobe.c
INCLUDE_PATH = -Isrc/fathom -Isrc/incbin

LIBS = -std=c++17 -pthread
OPTIM = -march=native -O3 -flto

ENGINE_OPTS = -DUSE_NN -DNN_FILE=\"$(EVALFILE)\" -DUSE_POPCNT -DIS_TUNING

ifneq ($(findstring g++, $(CXX)),)
	PGOGEN = -fprofile-generate
	PGOUSE = -fprofile-use
endif

ifneq ($(findstring clang, $(CXX)),)
	PGOMERGE = llvm-profdata merge -output=uralochka.profdata *.profraw
	PGOGEN = -fprofile-instr-generate
	PGOUSE = -fprofile-instr-use=uralochka.profdata
endif

CXX_FLAGS = $(OPTIM) $(LIBS) $(INCLUDE_PATH) $(ENGINE_OPTS)

all:
	$(CXX) $(CXX_FLAGS) $(PGOGEN) $(SRC) -o $(EXE)
	./$(EXE) bench
	$(PGOMERGE)
	$(CXX) $(CXX_FLAGS) $(PGOUSE) $(SRC) -o $(EXE)
	rm -f *.gcda *.profdata *.profraw
