#!/bin/bash

set -x

mkdir -p /root/uralochka3/train/build
cd /root/uralochka3/train/build

# https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
# sse2 -    core2
# avx2 -    haswell
# avx512 -  cannonlake

ARCH=haswell
if [[ -n "$1" ]]; then
    ARCH=$1
fi

cmake -DCMAKE_BUILD_TYPE=Release -DARCH=$ARCH ..

make -j

chmod -R 777 .
