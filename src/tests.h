#ifndef TESTS_H
#define TESTS_H

#include "common.h"
#include "board.h"

class Tests
{
public:
    static void goPerft();
    static void goBench();

private:
    static u64 perft(int ply, int depth, Board &board);
};

#endif // TESTS_H
