#ifndef UCIENGINE_H
#define UCIENGINE_H

#ifdef USE_PSTREAMS

#include "pstreams/pstream.h"

#include <string>

class UciEngine
{
public:
    UciEngine(bool io_print = false, int parse_delay = 10);
    ~UciEngine();

    bool open(std::string cmd);
    void close();

    void uci();
    void isready();
    void ucinewgame();
    void position(std::string position);

    void set_hash(int value);
    void clear_hash();
    void set_threads(int value);
    void set_syzygy(std::string value);

    std::string go(int depth = 0, int time = 0, int nodes = 0, int wtime = 0, int btime = 0, int inc = 0, int movestogo = 0);
    void stop();

    int get_score();

    void send_string(std::string str);
    std::string get_output();


private:
    redi::pstream *_proc;

    bool _io_print;
    int _parse_delay;

    int _score;

    static std::string substring(std::string &string, std::string delimiter = " ");
};

#endif

#endif // UCIENGINE_H
