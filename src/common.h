#ifndef COMMON_H
#define COMMON_H

#include <chrono>
#include <atomic>

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;

class Timer
{
public:
    Timer();
    void start();
    u32 get();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_point;
};

class Randomizer
{
public:
    Randomizer(u64 seed = 1070372ull);
    u64 random();
    float random01();

private:
    u64 _seed;
};

class SpinLock
{
public:
    void lock();
    void unlock();

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};

#endif // COMMON_H
