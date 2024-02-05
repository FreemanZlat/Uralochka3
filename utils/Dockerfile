FROM ubuntu:20.04

# RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y autoconf automake autopoint bash bison bzip2 flex g++ g++-multilib gettext git gperf intltool libc6-dev-i386 libgdk-pixbuf2.0-dev libltdl-dev libssl-dev libtool-bin libxml-parser-perl lzip make openssl p7zip-full patch perl python ruby sed unzip wget xz-utils cmake wine

# RUN cd /root && git clone https://github.com/mxe/mxe.git && cd mxe && git checkout build-2022-01-13
# RUN cd /root/mxe && make MXE_TARGETS=x86_64-w64-mingw32.static MXE_PLUGIN_DIRS=plugins/gcc8 cc cmake -j10 JOBS=10

RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get install -y wget xz-utils cmake wine

RUN cd /root && \
    wget https://github.com/mstorsjo/llvm-mingw/releases/download/20231017/llvm-mingw-20231017-msvcrt-ubuntu-20.04-x86_64.tar.xz && \
    tar xvf llvm-mingw-20231017-msvcrt-ubuntu-20.04-x86_64.tar.xz && \
    rm llvm-mingw-20231017-msvcrt-ubuntu-20.04-x86_64.tar.xz
