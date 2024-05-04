#ifndef UCI_H
#define UCI_H

#include "game.h"

#include <string>
#include <vector>
#include <thread>

class UCI
{
public:
    enum Type
    {
        EXACT,
        UPPERBOUND,
        LOWERBOUND
    };

    UCI();
    void start(std::string version = "Uralochka3");
    void non_uci(std::string input);

    void info(int depth, int sel_depth, int time, int score, Type type, std::string pv);
    void info(int depth, int move_number, std::string move);
    void info(int time);
    void sync(int time);

    void set_threads(int threads);
    void newgame();
    void set_startpos();
    void set_fen(std::string fen);
    void make_move(std::string move);
    void go(Rules rules, bool print_uci = true);
    void stop();
    void set_best(std::string bestmove, int score);
    std::string get_bestmove();
    int get_score();

private:
    static std::string substring(std::string &string, std::string delimiter = " ");

    std::vector<Game> _games;
    std::thread _game_thread;

    float _depth_average;
    int _depth_count;
};

#endif // UCI_H
