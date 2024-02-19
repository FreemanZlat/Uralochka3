#include "uci.h"

#include "tests.h"
#include "train_utils.h"
#include "hash.h"
#include "syzygy.h"
#include "neural.h"
#include "tuning_params.h"

#include <iostream>
#include <algorithm>

// Спецификация протокола: http://wbec-ridderkerk.nl/html/UCIProtocol.html

static const int EVAL_SCALE = 260;    // 260;

//#define TESTING

UCI::UCI()
{
    this->_is960 = false;
    this->set_threads(1);
}

void UCI::start(std::string version)
{
#ifdef IS_TUNING
    TuningParams &params = TuningParams::instance();
    for (const auto &param : params._params)
        if (param.need_tuning)
            std::cout << param.name << ", float, " << param.value << ", " << param.min << ", " << param.max << ", " << param.c_end << ", " << param.r_end << std::endl;
#endif

    while(true)
    {
        // Читаем строку
        std::string input;
        std::getline(std::cin, input);

        std::string input_full = input;

        // Парсим пришедшую строку. Первым словом идёт команда
        std::string cmd = UCI::substring(input);
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
        if (cmd == "uci")
        {
            // Отправляем инфу о движке
            std::cout << "id name " << version << std::endl;
            std::cout << "id author Ivan Maklyakov" << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max 512" << std::endl;
            std::cout << "option name Hash type spin default 32 min 8 max 65536" << std::endl;
            std::cout << "option name Clear Hash type button" << std::endl;
            std::cout << "option name SyzygyPath type string default " << std::endl;
            std::cout << "option name SyzygyProbeDepth type spin default 0 min 0 max 63" << std::endl;
            // std::cout << "option name UCI_Chess960 type check default false" << std::endl;
#ifdef IS_TUNING
            for (const auto &param : params._params)
                std::cout << "option name " << param.name << " type string default " << param.value << std::endl;
#endif
            // Всю инфу передали
            std::cout << "uciok" << std::endl;
        }
        else if (cmd == "isready")
        {
            // Вы готовы дети?!!! ДА КАПИТАН!!!!
            std::cout << "readyok" << std::endl;
        }
        else if (cmd == "setoption")
        {
            // Пример работы с опциями - обрабатываем получение нового размера хэша и нажатие кнопки очистки
//            std::transform (input.begin(), input.end(), input.begin(), ::tolower);
            UCI::substring(input);  // пропускаем слово name
            std::string option = UCI::substring(input);
            if (option == "Threads")
            {
                UCI::substring(input);  // пропускаем слово value
                this->set_threads(std::stoi(UCI::substring(input)));
            }
            else if (option == "Hash")
            {
                UCI::substring(input);  // пропускаем слово value
                TranspositionTable &table = TranspositionTable::instance();
                table.init(std::stoi(UCI::substring(input)));
            }
            else if (option == "Clear" && input == "Hash")
            {
                TranspositionTable &table = TranspositionTable::instance();
                table.clear();
            }
            else if (option == "SyzygyPath")
            {
                UCI::substring(input);  // пропускаем слово value
                Syzygy &egtb = Syzygy::instance();
                egtb.free();
                egtb.init(input);
            }
            else if (option == "SyzygyProbeDepth")
            {
                UCI::substring(input);  // пропускаем слово value
                Syzygy &egtb = Syzygy::instance();
                egtb.set_depth(std::stoi(UCI::substring(input)));
            }
/*
            else if (option == "UCI_Chess960")
            {
                UCI::substring(input);  // пропускаем слово value
                std::transform(input.begin(), input.end(), input.begin(), ::tolower);
                this->_is960 = (input == "true");
                for (auto &game : this->_games)
                    game.set_960(this->_is960);
            }
*/
#ifdef IS_TUNING
            else
            {
                UCI::substring(input);  // пропускаем слово value
                double value = std::stod(UCI::substring(input));
                if (!params.set(option, value))
                    std::cout << "info WARNING: no option: " << option << "!" << std::endl;
            }
#endif
        }
        else if (cmd == "ucinewgame")
        {
            this->newgame();
        }
        else if (cmd == "position")
        {
            // Установка позиции
            std::string from = UCI::substring(input);
            if (from == "startpos")
            {
                this->set_startpos();
                UCI::substring(input);  // moves
            }
            else    // В этом случае from должен быть "fen"
            {
                // Считываем позицию в формате FEN
                std::string fen = UCI::substring(input, " moves ");
                std::cout << "info string fen: " << fen << std::endl;
                this->set_fen(fen);
            }
            // Теперь считываем и обрабатываем ходы (если они есть) из указанной позиции
            std::string move = UCI::substring(input);
            while (move != "")
            {
                this->make_move(move);
                move = UCI::substring(input);
            }
        }
        else if (cmd == "go")
        {
            // Парсим условия
            Rules rules;
            while (input != "")
            {
                std::string rule = UCI::substring(input);
                if (rule == "depth")            rules._depth = std::stoi(UCI::substring(input));
                else if (rule == "movetime")    rules._movetime = std::stoi(UCI::substring(input));
                else if (rule == "wtime")       rules._wtime = std::stoi(UCI::substring(input));
                else if (rule == "btime")       rules._btime = std::stoi(UCI::substring(input));
                else if (rule == "winc")        rules._winc = std::stoi(UCI::substring(input));
                else if (rule == "binc")        rules._binc = std::stoi(UCI::substring(input));
                else if (rule == "movestogo")   rules._movestogo = std::stoi(UCI::substring(input));
                else if (rule == "mate")        rules._mate = std::stoi(UCI::substring(input));
                else if (rule == "infinite")    rules._infinite = true;
                else if (rule == "nodes")       rules._nodes = std::stoi(UCI::substring(input));
                else if (rule == "ponder")      rules._ponder = std::stoi(UCI::substring(input)) != 0;
            }
            // Запускаем думалку. По-хорошему, в отдельном потоке, чтоб корректно ловить и обрабатывать команду stop
            if (this->_game_thread.joinable())
                this->_game_thread.join();
            this->_game_thread = std::thread(&UCI::go, this, rules, true);
        }
        else if (cmd == "stop")
        {
            // Если процедура go в отдельном потоке, нужно ей сказать, что хватит думать, нужно ходить
            this->stop();
            this->_game_thread.join();
        }
        else if (cmd == "ponderhit")
        {
            // Если думали во время хода противника и он сделал предсказанный нами ход
        }
        else if (cmd == "quit" || cmd == "exit")
        {
            if (this->_game_thread.joinable())
                this->_game_thread.join();
            // Выходим из цикла
            break;
        }
        else
            this->non_uci(input_full);
    }
}

void UCI::non_uci(std::string input)
{
    while (!input.empty())
    {

        std::string cmd = UCI::substring(input);
        std::transform (cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "test")
        {
            Tests::goPerft();
        }
        else if (cmd == "bench")
        {
            // OpenBench-style bench :)
            if (input.empty())
            {
                TranspositionTable &table = TranspositionTable::instance();
                table.init(32);
                Tests::goBench();
            }
            else
            {
                int threads = 1;
                int time = 5;
                int hash = 128;

                if (!input.empty())
                    threads = std::stoi(UCI::substring(input));
                if (!input.empty())
                    time = std::stoi(UCI::substring(input));
                if (!input.empty())
                    hash = std::stoi(UCI::substring(input));

                std::cout << "bench threads=" << threads << " time=" << time << " hash=" << hash << std::endl;

                TranspositionTable &table = TranspositionTable::instance();
                if (hash > 0)
                    table.init(hash);
                table.clear();

                this->set_threads(threads);

                Rules rules;
                rules._movetime = time * 1000 + 100;
                this->go(rules);

                this->set_threads(1);
            }
        }
#ifdef USE_CNPY
        else if (cmd == "dg")
        {
            int threads = 16;
            int files = 1;
            int file_idx = 0;
            int hash1 = 4096;
            int hash2 = 4096;
            std::string book = "";
            std::string enemy = "";
            int enemy_depth = 7;
            int enemy_time = 0;
            int enemy_nodes = 0;

            if (!input.empty())
                threads = std::stoi(UCI::substring(input));
            if (!input.empty())
                files = std::stoi(UCI::substring(input));
            if (!input.empty())
                file_idx = std::stoi(UCI::substring(input));
            if (!input.empty())
                hash1 = std::stoi(UCI::substring(input));
            if (!input.empty())
                hash2 = std::stoi(UCI::substring(input));
            if (!input.empty())
                book = UCI::substring(input);
            if (!input.empty())
                enemy = UCI::substring(input);
            if (!input.empty())
                enemy_depth = std::stoi(UCI::substring(input));
            if (!input.empty())
                enemy_time = std::stoi(UCI::substring(input));
            if (!input.empty())
                enemy_nodes = std::stoi(UCI::substring(input));

            std::cout << "dg threads=" << threads << " files=" << files << " file_idx=" << file_idx;
            std::cout << " hash1=" << hash1 << " hash2=" << hash2 << " book='" << book << "'";
            std::cout << " enemy='" << enemy << "' enemy_depth=" << enemy_depth;
            std::cout << " enemy_time=" << enemy_time << " enemy_nodes=" << enemy_nodes << std::endl;

            DataGen generator;
            generator.init(threads, hash1, hash2, book);

            if (enemy != "")
                generator.set_enemy(enemy, enemy_depth, enemy_time, enemy_nodes);

            generator.gen("data/dg", files, file_idx);

            std::cout << "done" << std::endl;
        }
        else if (cmd == "convert")
        {
            int threads = 16;
            std::string in_file = "dg_0.npy";
            std::string out_file = "_dg_0.npy";

            if (!input.empty())
                threads = std::stoi(UCI::substring(input));
            if (!input.empty())
                in_file = UCI::substring(input);
            if (!input.empty())
                out_file = UCI::substring(input);

            std::cout << "convert threads=" << threads << " in_file=" << in_file << " out_file=" << out_file << std::endl;

            DataGen generator;
            generator.convert(threads, in_file, out_file);

            std::cout << "done" << std::endl;
        }
#endif
        else if (cmd == "book")
        {
            int threads = 1;
            int hash1 = 4096;
            int hash2 = 4096;
            int book_depth = 2;
            int depth = 10;
            int eval_from = 0;
            int eval_to = 50;
            std::string filename = "book.epd";

            if (!input.empty())
                threads = std::stoi(UCI::substring(input));
            if (!input.empty())
                hash1 = std::stoi(UCI::substring(input));
            if (!input.empty())
                hash2 = std::stoi(UCI::substring(input));
            if (!input.empty())
                book_depth = std::stoi(UCI::substring(input));
            if (!input.empty())
                depth = std::stoi(UCI::substring(input));
            if (!input.empty())
                eval_from = std::stoi(UCI::substring(input));
            if (!input.empty())
                eval_to = std::stoi(UCI::substring(input));
            if (!input.empty())
                filename = UCI::substring(input);

            std::cout << "book threads=" << threads << " hash1=" << hash1 << " hash2=" << hash2 << " book_depth=" << book_depth << " eval_depth=" << depth;
            std::cout << " eval_from=" << eval_from << " eval_to=" << eval_to << " filename='" << filename << "'" << std::endl;

            BookGen generator(threads);
            generator.gen(filename, hash1, hash2, book_depth, depth, 100*eval_from/EVAL_SCALE, 100*eval_to/EVAL_SCALE);

            std::cout << "done" << std::endl;
        }
        else if (cmd == "syz")
        {
            std::string path = UCI::substring(input);
            Syzygy &egtb = Syzygy::instance();
            egtb.free();
            if (!path.empty())
                egtb.init(path);
        }
        else if (cmd == "eval")
        {
        }
        else if (cmd == "nn")
        {
            std::string file = UCI::substring(input);
            Model::instance().init(file);
        }
#ifdef USE_PSTREAMS
        else if (cmd == "duel")
        {
            std::string engine = "";
            int games = 1;
            int self_time = 10000;
            int enemy_time = 10000;
            int inc = 100;
            int depth  = 0;
            std::string book = "";
            int book_seed = 0;
            int hash = 128;
            int threads = 1;
            std::string syz = "";


            if (!input.empty())
                engine = UCI::substring(input);
            if (!input.empty())
                games = std::stoi(UCI::substring(input));
            if (!input.empty())
                self_time = std::stoi(UCI::substring(input));
            if (!input.empty())
                enemy_time = std::stoi(UCI::substring(input));
            if (!input.empty())
                inc = std::stoi(UCI::substring(input));
            if (!input.empty())
                depth = std::stoi(UCI::substring(input));
            if (!input.empty())
                book = UCI::substring(input);
            if (!input.empty())
                book_seed = std::stoi(UCI::substring(input));
            if (!input.empty())
                hash = std::stoi(UCI::substring(input));
            if (!input.empty())
                threads = std::stoi(UCI::substring(input));
            if (!input.empty())
                syz = UCI::substring(input);

            std::cout << "duel engine='" << engine << "' games=" << games << " self_time=" << self_time;
            std::cout << " enemy_time=" << enemy_time << " inc=" << inc << " depth=" << depth;
            std::cout << " epd_book='" << book << "' book_seed=" << book_seed << " hash=" << hash;
            std::cout << " threads=" << threads << " syzygy_path='" << syz << "'" << std::endl;

            TranspositionTable &table = TranspositionTable::instance();
            if (hash > 0)
                table.init(hash);
            table.clear();

            this->set_threads(threads);

            Syzygy &egtb = Syzygy::instance();
            egtb.free();
            if (!syz.empty())
                egtb.init(syz);

            Duel duel(this);
            duel.open_engine(engine, hash, threads, syz);
            duel.fight(games, self_time, enemy_time, inc, depth, book, book_seed);
            duel.print_results();
        }
#endif
        else if (cmd == "help")
        {
            std::cout << "Additional commands:" << std::endl;
            std::cout << "- test                                                                                - do perft" << std::endl;
            std::cout << "- bench <threads=1> <time=5> <hash=128>                                               - do benchmark" << std::endl;
#ifdef USE_CNPY
            std::cout << "- dg <threads=16> <files=1> <start_idx=0> <hash1=4096> <hash2=4096> <epd_book=''>" << std::endl;
            std::cout << "     <enemy_engine=''> <enemy_depth=7> <enemy_time=0> <enemy_nodes=0>                 - do dataset generation" << std::endl;
            std::cout << "- convert <threads=16> <in_file> <out_file>                                           - convert to new dataset format" << std::endl;
#endif
            std::cout << "- book <threads=16> <hash1=4096> <hash2=4096> <book_depth=6>" << std::endl;
            std::cout << "       <eval_depth=10> <eval_from=0> <eval_to=50> <filename='book.epd'>               - do epd book generation" << std::endl;
            std::cout << "- syz <path=''>                                                                       - set syzygy path" << std::endl;
            std::cout << "- eval <fen=current>                                                                  - evaluate position" << std::endl;
            std::cout << "- nn <file=''>                                                                        - load nn from file" << std::endl;
#ifdef USE_PSTREAMS
            std::cout << "- duel <engine=''> <games=1> <self_time=10000> <enemy_time=10000> <inc=100> <depth=0>" << std::endl;
            std::cout << "       <epd_book=''> <book_seed=0> <hash=128> <threads=1> <syzygy_path=''>            - duel vs external uci engine" << std::endl;
#endif
        }
    }
}

void UCI::info(int depth, int sel_depth, int time, int score, Type type, std::string pv)
{
#ifndef TESTING
    if (time == 0)
        time = 1;

    u64 nodes = 0;
    u64 tbhits = 0;
    for (auto &game : this->_games)
    {
        nodes += game._nodes;
        tbhits += game._tbhits;
    }

    u64 nps = 1000 * nodes / time;

    std::cout << "info depth "+ std::to_string(depth) <<
                 (sel_depth != 0 ? " seldepth " + std::to_string(sel_depth) : "") <<
                 (time != 0 ? " time " + std::to_string(time) : "") <<
                 (nodes != 0 ? " nodes " + std::to_string(nodes) : "") <<
                 (nps != 0 ? " nps " + std::to_string(nps) : "") <<
                 " tbhits " << tbhits <<
                 " score " + (std::abs(score) > 19000 ?
                                  "mate " + (score > 0 ? std::to_string((20001-score) / 2) : "-" + std::to_string((20001+score) / 2)) :
                                  "cp " + std::to_string(100 * score / EVAL_SCALE)) <<
                 (type == UCI::UPPERBOUND ? " upperbound" : type == UCI::LOWERBOUND ? " lowerbound" : "") <<
                 (pv != "" ? " pv " + pv : "") <<
                 std::endl;
#endif
}

void UCI::info(int depth, int move_number, std::string move)
{
#ifndef TESTING
    std::cout << "info depth " << depth << " currmovenumber " << move_number << " currmove " << move << std::endl;
#endif
}

void UCI::info(int time)
{
#ifndef TESTING
    if (time == 0)
        time = 1;

    u64 nodes = 0;
    u64 tbhits = 0;
    for (auto &game : this->_games)
    {
        nodes += game._nodes;
        tbhits += game._tbhits;
    }

    TranspositionTable &table = TranspositionTable::instance();
    u64 nps = 1000 * nodes / time;
    std::cout << "info time " << time << " nodes " << nodes << " nps " << nps << " tbhits " << tbhits << " hashfull " << table.get_percent() << std::endl;
#endif
}

void UCI::sync(int time)
{
    if (this->_games[0]._time_max != 0)
    {
        int max_depth = 0;
        for (auto &game : this->_games)
        {
            int best_depth = game._best_depth;
            if (best_depth > max_depth)
                max_depth = best_depth;
        }
        for (auto &game : this->_games)
            game._depth_best = max_depth;
    }
}

void UCI::set_threads(int threads)
{
    this->_games.clear();
    this->_games.resize(threads);
    for (auto &game : this->_games)
    {
        game.set_uci(this);
        game.set_960(this->_is960);
    }
    std::cout << "info string Threads count: " << threads << std::endl;
}

void UCI::newgame()
{
    // Ставим стартовую позицию и очищаем хэш
    this->set_startpos();
    for (auto &game : this->_games)
    {
        game._best_value = 0;
        game._best_depth = 0;
        game._depth_best = 0;
        game._depth_prev = 0;
    }

    this->_depth_average = 0;
    this->_depth_count = 0;

    TranspositionTable &table = TranspositionTable::instance();
    table.clear();
}

void UCI::set_startpos()
{
    for (auto &game : this->_games)
        game.set_startpos();
}

void UCI::set_fen(std::string fen)
{
    for (auto &game : this->_games)
        game.set_fen(fen);
}

void UCI::make_move(std::string move)
{
    for (auto &game : this->_games)
        game.make_move(move);
}

void UCI::go(Rules rules, bool print_uci)
{
    Timer timer;

    for (auto &game : this->_games)
    {
        game._best_depth = 0;
        game._depth_best = 0;
        game._nodes = 0;
        game._is_cancel = false;
    }

    u16 best_move = 0;
    if (this->_games[0].egtb_dtz(best_move))
    {
        if (print_uci)
        {
            this->info(timer.get());
            std::cout << "bestmove " << Move::get_string(best_move) << std::endl;
        }
        this->set_best(this->_games[0]._best_move, this->_games[0]._best_value);
        return;
    }

    std::vector<std::thread> threads;
    threads.push_back(std::thread(&Game::go_rules, &this->_games[0], rules, print_uci));

    for (int i = 1; i < this->_games.size(); ++i)
        threads.push_back(std::thread(&Game::go, &this->_games[i]));

    threads[0].join();

    for (int i = 1; i < this->_games.size(); ++i)
        this->_games[i].stop();

    for (int i = 1; i < this->_games.size(); ++i)
        threads[i].join();

    int best = 0;
    for (int i = 1; i < this->_games.size(); ++i)
    {
        int best_depth = this->_games[0]._best_depth;
        int best_score = this->_games[0]._best_value;

        int this_depth = this->_games[i]._best_depth;
        int this_score = this->_games[i]._best_value;

        if (this_depth == 0)
            continue;

        if ((this_depth == best_depth && this_score > best_score) || (this_score > 19000 && this_score > best_score))
            best = i;

        if (this_depth > best_depth && (this_score > best_score || best_score < 19000))
            best = i;
    }

    if (this->_games[0]._time_max != 0)
    {
        int depth = this->_games[best]._best_depth;
        int depth_av = this->_depth_count == 0 ? 20 : this->_depth_average;

        if (std::abs(depth_av - depth) > 8)
            depth = depth_av;
        else
        {
            this->_depth_average = (this->_depth_average * this->_depth_count + depth) / (this->_depth_count + 1);
            if (this->_depth_count < 10)
                this->_depth_count++;
        }

        for (auto &game : this->_games)
        {
            game._depth_prev = 1 + depth * 3 / 4;
            game._depth_time = std::max(12, std::max(game._depth_prev, depth - 4));
        }
    }

    this->set_best(this->_games[best]._best_move, this->_games[best]._best_value);

    if (print_uci)
    {
#ifdef TESTING
        std::cout << "info depth " << this->_games[best]._best_depth << " time " << timer.get() <<
                     " score cp " << this->_games[best]._best_value << " pv " << this->_games[best]._best_pv << std::endl;
#else
        if (best > 0)
        {
//            std::cout << "info string Best move is not from main thread" << std::endl;
            this->info(this->_games[best]._best_depth, this->_games[best]._sel_depth, timer.get(), this->_games[best]._best_value, UCI::EXACT, this->_games[best]._best_pv);
        }
#endif

        this->info(timer.get());
        // Выводим лучший ход
        std::cout << "bestmove " << this->_games[best]._best_move << std::endl;
    }
}

void UCI::stop()
{
    for (auto &game : this->_games)
        game.stop();
}

void UCI::set_best(std::string bestmove, int score)
{
    for (auto &game : this->_games)
    {
        game._best_move = bestmove;
        game._best_value = score;
    }
}

std::string UCI::get_bestmove()
{
    return this->_games[0]._best_move;
}

int UCI::get_score()
{
    return this->_games[0]._best_value;
}

std::string UCI::substring(std::string &string, std::string delimiter)
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
