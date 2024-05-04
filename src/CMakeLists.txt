cmake_minimum_required(VERSION 3.5)

project(Uralochka3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
# sse2 -    core2
# avx2 -    haswell
# avx512 -  cannonlake
set(ARCH "native" CACHE STRING "Architecture to tell gcc to optimize for (-march)")
set(USE_NN ON CACHE BOOL "Use NNUE-like neural network")
set(NN_FILE "nn_1.2f_e500_l300_d400.nn" CACHE STRING "NN file :-)")
set(USE_PEXT OFF CACHE BOOL "Use PEXT")
set(USE_POPCNT OFF CACHE BOOL "Use POPCNT")
set(USE_CNPY OFF CACHE BOOL "Use CNPY for datagen output")
set(USE_PSTREAMS OFF CACHE BOOL "Use PSTREAMS for UciEngine")
set(PGO_GENERATE OFF CACHE BOOL "Generate PGO")
set(PGO_USE OFF CACHE BOOL "Use PGO")
set(PGO_USE_FILE "" CACHE STRING "Use PGO file")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCH} -static -flto")

if(PGO_GENERATE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate")
    endif()
elseif(PGO_USE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-use=${PGO_USE_FILE}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-correction -fprofile-use")
    endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")
endif ()

file(GLOB URALOCHKA_3_SRC *.cpp)
file(GLOB URALOCHKA_3_HDR *.h)

set(FATHOM_SRC fathom/tbprobe.c)
file(GLOB FATHOM_HDR fathom/*.h)

set(INCBIN_HDR incbin/incbin.h)

include_directories(fathom)
include_directories(incbin)

if(USE_PEXT)
    add_definitions(-DUSE_PEXT)
endif()

if(USE_POPCNT)
    add_definitions(-DUSE_POPCNT)
endif()

if(USE_CNPY)
    add_definitions(-DUSE_CNPY)
    file(GLOB CNPY_SRC cnpy/*.cpp)
    file(GLOB CNPY_HDR cnpy/*.h)
    include_directories(cnpy)
endif()

if(USE_PSTREAMS)
    add_definitions(-DUSE_PSTREAMS)
    set(PSTREAMS_HDR pstreams/pstream.h)
endif()

if(USE_NN)
    add_definitions(-DUSE_NN)
    if(NOT NN_FILE STREQUAL "")
        add_definitions(-DNN_FILE="${NN_FILE}")
    endif()
endif()

add_executable(Uralochka3 ${URALOCHKA_3_SRC} ${URALOCHKA_3_HDR} ${FATHOM_SRC} ${FATHOM_HDR} ${CNPY_SRC} ${CNPY_HDR} ${INCBIN_HDR} ${PSTREAMS_HDR})

if(LINUX_STATIC)
    target_link_libraries(Uralochka3 -Wl,--whole-archive rt pthread -Wl,--no-whole-archive)
endif()
