# if git-lfs is installed, it should clone everything, including the neural network file
git clone https://gitlab.com/freemanzlat/uralochka3.git

# get the latest version
cd uralochka3
git pull origin main
# just in case
git lfs pull
cd ..

chmod +x uralochka3/utils/build_ur3.sh

docker run --rm -i -v ./uralochka3:/root/uralochka3 silkeh/clang:20-bullseye /root/uralochka3/utils/build_ur3.sh linux 1 /root/uralochka3/src /root/uralochka3/build "/root/uralochka3/nn/*.nn" "-DCMAKE_BUILD_TYPE=Release -DARCH=native -DUSE_POPCNT=1 -DUSE_PEXT=1 -DLINUX_STATIC=1"

mv uralochka3/build/Uralochka3 .
rm -rf uralochka3/build

EXE=$PWD/Uralochka3 
