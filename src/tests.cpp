#include "tests.h"

#include "hash.h"
#include "game.h"

#include <vector>
#include <string>

// https://www.chessprogramming.org/Perft_Result

struct PerftTest
{
    std::string fen;
    int depth;
    std::vector<u64> results;
};

static const std::vector<PerftTest> &TESTS =
{
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 16, { 20, 400, 8902, 197281, 4865609, 119060324 } },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 21, { 48, 2039, 97862, 4085603, 193690690 } },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 19, { 14, 191, 2812, 43238, 674624, 11030083 } },
    { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 16, { 6, 264, 9467, 422333, 15833292 } },
    { "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 16, { 6, 264, 9467, 422333, 15833292 } },
    { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 17, { 44, 1486, 62379, 2103487, 89941194 } },
    { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 18, { 46, 2079, 89890, 3894594, 164075551 } },
    { "rn1qkbnr/p1ppppp1/7P/1p6/7P/8/PPPPPP2/RNBQKBNb w Qkq - 0 5 ", 18, { 21, 550, 12777, 351356, 8906367 } }
};


void Tests::goPerft()
{
    Board board;
    Timer timer;
    for (auto &test : TESTS)
    {
        board.set_fen(test.fen);
        board.print();
        int i = 1;
        for (auto result : test.results)
        {
            timer.start();
            u64 res = Tests::perft(0, i, board);
            auto time = timer.get();
            if (res == result)
                printf("%d.  res=%lld  t=%d\n", i++, res, time);
            else
                printf("%d.  res=%lld  ERROR! correct: %lld\n", i++, res, result);
        }
        printf("\n");
    }
}

void Tests::goBench()
{
    TranspositionTable &table = TranspositionTable::instance();

    u64 nodes_total = 0;
    u64 time_total = 0;

    printf("Start benchmark\n");
    for (auto &test : TESTS)
    {
        table.clear();

        Game game;
        game.set_fen(test.fen);

        Timer timer;
        u64 nodes = game.go_bench(test.depth);
        time_total += timer.get();

        printf("%s : nodes=%lld\n", test.fen.c_str(), nodes);
        nodes_total += nodes;
    }

    int nps = 1000ull * nodes_total / time_total;
    printf("%lld nodes %d nps\n", nodes_total, nps);
}

u64 Tests::perft(int ply, int depth, Board &board)
{
//    board.test();

    u64 res = 0;
    auto moves = board.moves_init(ply);

    u16 move = 0;
    while ((move = moves->get_next()) != 0)
    {
        if (!board.move_do(move, ply))
            continue;

        u64 move_res = 1;
        if (depth > 1)
            move_res = perft(ply+1, depth-1, board);
        res += move_res;
        board.move_undo(move, ply);
//        if (ply == 0)
//        {
//            std::string move_str = Move::get_string(move);
//            printf("%s: %lld\n", move_str.c_str(), move_res);
//        }
    }
    board.moves_free();

    return res;
}
