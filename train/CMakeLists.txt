cmake_minimum_required(VERSION 3.5)

project(uralochka3_batch LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
# sse2 -    core2
# avx2 -    haswell
# avx512 -  cannonlake
set(ARCH "native" CACHE STRING "Architecture to tell gcc to optimize for (-march)")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCH} -flto")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")
endif ()

file(GLOB CNPY_SRC ../src/cnpy/*.cpp)
file(GLOB CNPY_HDR ../src/cnpy/*.h)
include_directories(../src/cnpy)

add_library(uralochka3_batch SHARED
  uralochka3_batch.cpp
  ndarray.h
  ${CNPY_SRC}
  ${CNPY_HDR}
)

target_compile_definitions(uralochka3_batch PRIVATE URALOCHKA3_BATCH_LIBRARY)
