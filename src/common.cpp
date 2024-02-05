#include "common.h"

Timer::Timer()
{
    this->start();
}

void Timer::start()
{
    this->start_point = std::chrono::high_resolution_clock::now();
}

u32 Timer::get()
{
    auto current = std::chrono::high_resolution_clock::now();
    return (u32) std::chrono::duration_cast<std::chrono::milliseconds>(current - this->start_point).count();
}

Randomizer::Randomizer(u64 seed):
    _seed(seed)
{
}

u64 Randomizer::random()
{
    this->_seed ^= this->_seed >> 12;
    this->_seed ^= this->_seed << 25;
    this->_seed ^= this->_seed >> 27;
    return this->_seed * 2685821657736338717ull;
}

float Randomizer::random01()
{
    u64 tmp = this->random() % 65536;
    return static_cast<float>(tmp) / 65536.0f;
}

void SpinLock::lock()
{
//    while (locked.test_and_set(std::memory_order_acquire)) { ; }
}

void SpinLock::unlock()
{
//    locked.clear(std::memory_order_release);
}

