#include "uciengine.h"

#include <chrono>
#include <iostream>
#include <thread>

#ifdef USE_PSTREAMS

UciEngine::UciEngine(bool io_print, int parse_delay)
{
    this->_proc = nullptr;
    this->_io_print = io_print;
    this->_parse_delay = parse_delay;
    this->_score = 0;
}

UciEngine::~UciEngine()
{
    this->close();
}

bool UciEngine::open(std::string cmd)
{
    this->_proc = new redi::pstream(cmd, redi::pstreams::pstdout | redi::pstreams::pstdin);
    // handle errors? no! :-D
    return true;
}

void UciEngine::close()
{
    this->send_string("quit");

    if (this->_proc)
    {
        delete this->_proc;
        this->_proc = nullptr;
    }
}

void UciEngine::uci()
{
    this->send_string("uci");
    bool is_exit = false;
    while (!is_exit)
    {
        auto buf = this->get_output();
        if (buf == "" && this->_parse_delay > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->_parse_delay));
            continue;
        }
        while (buf != "")
        {
            auto line = UciEngine::substring(buf, "\n");
            if (this->_io_print)
                std::cout << ">> " << line << std::endl;

            if (line == "uciok")
            {
                is_exit = true;
                break;
            }
        }
    }
}

void UciEngine::isready()
{
    this->send_string("isready");
    bool is_exit = false;
    while (!is_exit)
    {
        auto buf = this->get_output();
        if (buf == "" && this->_parse_delay > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->_parse_delay));
            continue;
        }
        while (buf != "")
        {
            auto line = UciEngine::substring(buf, "\n");
            if (this->_io_print)
                std::cout << ">> " << line << std::endl;

            if (line == "readyok")
            {
                is_exit = true;
                break;
            }
        }
    }
}

void UciEngine::ucinewgame()
{
    this->send_string("ucinewgame");
}

void UciEngine::position(std::string position)
{
    this->send_string("position " + position);
}

void UciEngine::set_hash(int value)
{
    this->send_string("setoption name Hash value " + std::to_string(value));
}

void UciEngine::clear_hash()
{
    this->send_string("setoption name Clear Hash");
}

void UciEngine::set_threads(int value)
{
    this->send_string("setoption name Threads value " + std::to_string(value));
}

void UciEngine::set_syzygy(std::string value)
{
    this->send_string("setoption name SyzygyPath value " + value);
}

std::string UciEngine::go(int depth, int time, int nodes, int wtime, int btime, int inc, int movestogo)
{
    std::string rule = "";

    if (depth != 0)
        rule = "depth " + std::to_string(depth);
    else if (nodes != 0)
        rule = "nodes " + std::to_string(nodes);
    else if (time != 0)
        rule = "movetime " + std::to_string(time);
    else if (wtime != 0 && btime != 0)
    {
        rule = "wtime " + std::to_string(wtime) + " btime " + std::to_string(btime);
        if (inc != 0)
            rule += " winc " + std::to_string(inc) + " binc " + std::to_string(inc);
        if (movestogo != 0)
            rule += " movestogo " + std::to_string(movestogo);
    }
    else
        rule = "infinite";

    this->send_string("go " + rule);

    std::string bestmove = "";
    while (bestmove == "")
    {
        std::string buf = this->get_output();
        if (buf == "" && this->_parse_delay > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(this->_parse_delay));
            continue;
        }
        while (buf != "")
        {
            auto line = UciEngine::substring(buf, "\n");
            if (this->_io_print)
                std::cout << ">> " << line << std::endl;

            auto word = UciEngine::substring(line);
            if (word == "bestmove")
            {
                bestmove = UciEngine::substring(line);
                break;
            }

            if (word == "info")
            {
                while (line != "")
                {
                    word = UciEngine::substring(line);
                    if (word == "pv")
                        break;
                    if (word == "score")
                    {
                        word = UciEngine::substring(line);
                        std::string val = UciEngine::substring(line);
                        if (word == "" || val == "")
                            break;
                        this->_score = std::stoi(val);
                        if (word == "mate")
                        {
                            if (this->_score > 0)
                                this->_score = 20000 - this->_score;
                            else
                                this->_score = -20000 - this->_score;
                        }
                        break;
                    }
                }
            }
        }
    }

    return bestmove;
}

void UciEngine::stop()
{
    this->send_string("stop");
}

int UciEngine::get_score()
{
    return this->_score;
}

void UciEngine::send_string(std::string str)
{
    if (!this->_proc)
        return;
    if (this->_io_print)
        std::cout << "<< " << str << std::endl;
    *this->_proc << str << std::endl;
}

std::string UciEngine::get_output()
{
    std::string res;

    char buf[4096];
    std::streamsize n;

    while ((n = this->_proc->out().readsome(buf, sizeof(buf))) > 0)
        res.append(buf, n);

    return res;
}

std::string UciEngine::substring(std::string &string, std::string delimiter)
{
    size_t pos = 0;
    std::string result;
    if ((pos = string.find(delimiter)) != std::string::npos)
    {
        result = string.substr(0, pos);
        string.erase(0, pos + delimiter.length());
    }
    else
    {
        result = string;
        string = "";
    }
    return result;
}

#endif
