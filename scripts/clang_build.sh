cp *.nn ../

# docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=haswell -DUSE_NN=0 -DUSE_POPCNT=1 -DUSE_PEXT=1 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"
# mv ../build/Uralochka3 Uralochka3-no_nn

docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=core2 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"
mv ../build/Uralochka3 Uralochka3-sse

docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=haswell -DUSE_POPCNT=1 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"
mv ../build/Uralochka3 Uralochka3-avx2

docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=cannonlake -DUSE_POPCNT=1 -DUSE_PEXT=1 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"
mv ../build/Uralochka3 Uralochka3-avx512

docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=native -DUSE_POPCNT=1 -DUSE_PEXT=1 -DLINUX_STATIC=1 -DUSE_CNPY=1 -DUSE_PSTREAMS=1"
mv ../build/Uralochka3 Uralochka3-best

# docker run --rm -i -v ~/repo/uralochka3:/root/uralochka3 registry.gitlab.com/freemanzlat/uralochka3/ur3win:20250709 /root/uralochka3/utils/build_ur3.sh win-clang 1 /root/uralochka3/src /root/uralochka3/build "../*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=haswell -DUSE_POPCNT=1"
# mv ../build/Uralochka3.exe Uralochka3-avx2.exe

rm ../*.nn
rm -rf ../build

./Uralochka3-sse bench
./Uralochka3-avx2 bench
./Uralochka3-avx512 bench
./Uralochka3-best bench

