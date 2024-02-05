#!/bin/bash

# set -x

echo "./build_ur3.sh <linux|win-mxe|win-clang> <use_pgo> <src_dir> <build_dir> <path_to_nn> <cmake_options>"

OS_TYPE=linux
PGO=0
SRC_DIR=../src
BUILD_DIR=/root/uralochka3/build
NN_FILE="../*.nn"
CMAKE_OPTS="-DCMAKE_BUILD_TYPE=Release -DARCH=haswell -DUSE_POPCNT=1 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"

if [[ -n "$1" ]]; then
    OS_TYPE=$1
fi

if [[ -n "$2" ]]; then
    PGO=$2
fi

if [[ -n "$3" ]]; then
    SRC_DIR=$3
fi

if [[ -n "$4" ]]; then
    BUILD_DIR=$4
fi

if [[ -n "$5" ]]; then
    NN_FILE=$5
fi

if [[ -n "$6" ]]; then
    CMAKE_OPTS=$6
fi

case $OS_TYPE in

linux)
    CMAKE=cmake
    EXE_FILE=./Uralochka3
    LLVM_PROFDATA=llvm-profdata
    WAIT_START=1
    ;;

win-clang)
    CMAKE="cmake -DCMAKE_C_COMPILER=x86_64-w64-mingw32-clang -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++"
    EXE_FILE="wine Uralochka3.exe"
    LLVM_PROFDATA=/root/llvm-mingw-20231017-msvcrt-ubuntu-20.04-x86_64/bin/llvm-profdata
    export PATH=/root/llvm-mingw-20231017-msvcrt-ubuntu-20.04-x86_64/bin:${PATH}
    WAIT_START=5
    ;;

win-mxe)
    CMAKE=x86_64-w64-mingw32.static-cmake
    EXE_FILE="wine Uralochka3.exe"
    LLVM_PROFDATA=
    export PATH=/root/mxe/usr/bin:${PATH}
    WAIT_START=5
    ;;

*)
    echo "Choose OS type: windows or linux"
    exit
    ;;
esac

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cp $NN_FILE .

if [[ "$PGO" == "0" ]]; then
    $CMAKE $CMAKE_OPTS $SRC_DIR
    make -j
else
    $CMAKE $CMAKE_OPTS -DPGO_GENERATE=1 $SRC_DIR
    make -j

    # $EXE_FILE bench 1 10 128
    (sleep $WAIT_START; echo "go movetime 8000"; sleep 10; echo "quit") | $EXE_FILE

    if [[ -n "$LLVM_PROFDATA" ]]; then
        $LLVM_PROFDATA merge -output=ur3.profdata *.profraw
    fi

    rm -rf CMake*
    rm -rf Make*
    rm -rf cmake*
    ls -lah

    $CMAKE $CMAKE_OPTS -DPGO_USE=1 -DPGO_USE_FILE=ur3.profdata $SRC_DIR
    make -j
fi

chmod -R 777 .
