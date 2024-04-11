#include "train_utils.h"

#include "hash.h"
#include "syzygy.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <thread>
#include <random>

#ifdef USE_CNPY
#include <cnpy.h>
#endif

static const double TEXEL_K = 1.13;
static const double IS_GG = 150.0;
static const double MOVES_GG = 50.0;
static const int PER_THREAD = 2000;

static const std::vector<std::vector<int>> &DELTAS = {
    { -2, -1, 1, 2 },
    { -1, 1 },
    { -4, -2, -1, 2 },
    { -2, 1, 2, 4 }
};

// Tuner

Tuner::Tuner(int threads)
{
    this->_games.clear();
    this->_games.resize(threads);
}

void Tuner::load(std::string filename, Type type, int max_size)
{
    this->_timer.start();

    EvalParams &params = EvalParams::instance();

    std::ifstream file(filename);

    std::string fen;
    this->_size = 0;
    while (std::getline(file, fen))
    {
        std::string line;
        std::getline(file, line);

        int moves;
        double res;
        int is_book = 0;
        double eval = 0;

        std::istringstream values(line);
        values >> moves >> res >> is_book >> eval;      // В fens6 нет is_book и eval

        if (type == Tuner::TUNE && moves > 100)
            continue;

        this->_games[0].set_fen(fen);

        if (type == Tuner::NEURAL)
        {
/*
            u64 wk = this->_games[0]._board._bitboards[0][Board::KING];
            u64 bk = this->_games[0]._board._bitboards[1][Board::KING];
            u64 wp = this->_games[0]._board._bitboards[0][Board::PAWN];
            u64 bp = this->_games[0]._board._bitboards[1][Board::PAWN];
            u64 wn = this->_games[0]._board._bitboards[0][Board::KNIGHT];
            u64 bn = this->_games[0]._board._bitboards[1][Board::KNIGHT];
            u64 wb = this->_games[0]._board._bitboards[0][Board::BISHOP];
            u64 bb = this->_games[0]._board._bitboards[1][Board::BISHOP];
            u64 wr = this->_games[0]._board._bitboards[0][Board::ROOK];
            u64 br = this->_games[0]._board._bitboards[1][Board::ROOK];
            u64 wq = this->_games[0]._board._bitboards[0][Board::QUEEN];
            u64 bq = this->_games[0]._board._bitboards[1][Board::QUEEN];
            this->_neural.add_node(wk, bk, wp, bp, wn, bn, wb, bb, wr, br, wq, bq, Neural::sigmoid(eval/150));
*/
        }
        else
        {
            if (is_book && type == Tuner::TUNE)
                continue;

            if (type == Tuner::TUNE)
            {
                int eval1 = this->_games[0].eval();
                int eval2 = this->_games[0].quiescence_tune(0, -20000, 20000);
                if (abs(eval1-eval2) > 50)
                    continue;
            }

            this->_fens.push_back(fen);
            this->_cnt_moves.push_back(moves);
            this->_results.push_back(res);
            this->_books.push_back(is_book);
        }

        if ((++this->_size % 100000) == 0)
            std::cout << "Loading: " << this->_size << std::endl;

        if (this->_size == max_size)
            break;
    }

    file.close();

    std::cout << "Loaded: " << this->_size << " (" << this->_timer.get() / 1000 << " seconds)" << std::endl;
}

double Tuner::eval()
{
    this->_threads_res = 0.0;
    this->_threads_current = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < this->_games.size(); ++i)
        threads.push_back(std::thread(&Tuner::eval_thread, this, i));

    for (int i = 0; i < this->_games.size(); ++i)
        threads[i].join();

    this->_threads_res /= static_cast<double>(this->_size);
    return this->_threads_res;
}

void Tuner::eval_thread(int thread_id)
{
    double res = 0.0;
    int start, size;
    while (this->eval_get(start, size, res))
    {
        res = 0.0;
        for (int i = 0; i < size; ++i)
        {
            this->_games[thread_id].set_fen(this->_fens[start+i]);

//            double eval = this->_game.quiescence_tune(0, -20000, 20000);
            double eval = this->_games[thread_id].eval();

            if (this->_games[thread_id]._board.color(0))
                eval = -eval;

            double r = this->_results[start+i] - 1.0 / (1.0 + pow(10.0, -this->_texel_k * eval / 400.0));
//            double r = this->_results[start+i] - 1.0 / (1.0 + exp(-eval/IS_GG));

            res += r*r;
//            res += r*r*exp(-this->_cnt_moves[start+i]/MOVES_GG);
        }
    }
}

bool Tuner::eval_get(int &start, int &size, double res, bool print)
{
    bool result = false;

    this->_lock1.lock();

    this->_threads_res += res;

    start = this->_threads_current;
    if (this->_threads_current < this->_size)
    {
        size = std::min(PER_THREAD, (int)this->_size - this->_threads_current);
        this->_threads_current += size;
        result = true;
    }

    if (print)
        std::cout << start << "/" << this->_size << " :  " << 100.0f * start / this->_size << "%" << std::endl;

    this->_lock1.unlock();

    return result;
}

void Tuner::compute_k()
{
    std::cout << "Computing K..." << std::endl;

    double start = -10.0, end = 10.1, step = 1.0;
    double best_eval = 100500.0;

    for (int i = 0; i < 5; ++i)
    {
        double current = start;

        while (current < end)
        {
            this->_texel_k = current;
            double eval = this->eval();
            if (eval < best_eval)
            {
                best_eval = eval;
                start = current;
            }
            current += step;
        }

        std::cout << i << ". best=" << best_eval << " k=" << start << std::endl;

        end = start + step;
        start -= step;
        step /= 10.0;
    }

    this->_texel_k = start;
}

void Tuner::start(std::string in, std::string out)
{
    EvalParams &params = EvalParams::instance();
    params.load(in);

    this->_texel_k = TEXEL_K;
//    this->compute_k();

    double eval = this->eval();
    std::cout << "start eval: " << eval << std::endl;

    int improved = 1;
    int iter = 0;

    while (improved > 0)
    {
        this->_timer.start();

        this->save(out, iter, eval);

        iter++;
        improved = 0;

        params.start();

        int *val = nullptr;
        int count = 0;
        std::cout << iter << "." << count << " ";
        while ((val = params.get_next()) != nullptr)    // or get_next_mat
        {
            int current_val = *val;
            int best_val = current_val;
            int best = 0;

            if (iter == 1)
                this->_params.push_back({ 0, 0, 0 });

            int deltas_idx = this->_params[count][2];

            if (this->_params[count][0] == 0 && this->_params[count][1] > 2 && (this->_params[count][1] % 3) < 2)
            {
                this->_params[count][1]++;
                std::cout << "= " << best_val << " (skip)  : " << eval << std::endl;
                std::cout << iter << "." << ++count << " ";
                continue;
            }

            for (int delta : DELTAS[deltas_idx])
            {
                *val = current_val + delta;
                params.init_pst();

                double new_eval = this->eval();
                if (new_eval < eval)
                {
                    eval = new_eval;
                    best_val = *val;
                    best = delta;
                }
            }

            if (best != 0)
                improved++;

            if (this->_params[count][0] == best)
                this->_params[count][1]++;
            else
            {
                this->_params[count][0] = best;
                this->_params[count][1] = 1;
            }

            if (best == 0)
                this->_params[count][2] = 1; // -1 1
            else if (best == -1 || best == 1)
                this->_params[count][2] = 0; // -2 -1 1 2
            else if (best == -2)
                this->_params[count][2] = 2; // -4 -2 -1 2
            else if (best == 2)
                this->_params[count][2] = 3; // -2 1 2 4

            *val = best_val;
            std::cout << "= " << best_val << " (" << (best>0 ? "+" : "") << best << ")  : " << eval << std::endl;
            std::cout << iter << "." << ++count << " ";
        }

        int seconds = static_cast<int>(this->_timer.get() / 1000);
        int minutes = seconds / 60;
        seconds = seconds % 60;

        std::cout << iter << ". Improved parameters: " << improved << ", Time: " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds << std::endl;
    }
}

void Tuner::eval_dataset(std::string out_file)
{
    this->_threads_current = 0;
    this->_evals.resize(this->_size);

    TranspositionTable &table = TranspositionTable::instance();
    table.init(1024);

    std::vector<std::thread> threads;
    for (int i = 0; i < this->_games.size(); ++i)
        threads.push_back(std::thread(&Tuner::dataset_thread, this, i));

    for (int i = 0; i < this->_games.size(); ++i)
        threads[i].join();

    std::ofstream file(out_file);
    for (int i = 0; i < this->_size; ++i)
    {
        file << this->_fens[i] << std::endl;
        file << this->_cnt_moves[i] << " " << this->_results[i] << " " << this->_books[i] << " " << this->_evals[i] << std::endl;
    }
    file.close();
}

void Tuner::dataset_thread(int thread_id)
{
    int start, size;
    while (this->eval_get(start, size, 0.0, true))
    {
        for (int i = 0; i < size; ++i)
        {
            int idx = start + size - 1 - i;
            this->_games[thread_id].set_fen(this->_fens[idx]);

            u16 best_move = 0;
            int eval = this->_games[thread_id].search_root(11, best_move);

            if (this->_games[thread_id]._board.color(0))
                eval = -eval;

            this->_evals[idx] = eval;
        }
    }
}

void Tuner::save(std::string out, int iter, double eval)
{
    EvalParams &params = EvalParams::instance();
    params.save(out + "." + std::to_string(iter));

    std::ofstream file("tuner.txt", std::ios_base::app);
    file << iter << ", " << eval << std::endl;
    file.close();
}

// HashTable

HashTable::HashTable()
{
    this->_hash = nullptr;
    this->_size = 0;
}

HashTable::~HashTable()
{
    if (this->_hash != nullptr)
        delete []this->_hash;
    this->_hash = nullptr;
    this->_size = 0;
}

void HashTable::resize(int size_mb)
{
    if (this->_hash != nullptr)
        delete []this->_hash;
    const u64 MB = 1ull << 20;
    this->_size = size_mb * MB / sizeof(u64);
    this->_hash = new u64[this->_size];
    for (u64 i = 0; i < this->_size; ++i)
        this->_hash[i] = 0;
//    std::cout << "Hash2 size: " << this->_size << std::endl;
}

bool HashTable::check_hash(u64 hash)
{
    u64 idx = hash % this->_size;
    u64 value = this->_hash[idx];
    if (hash == value)
        return true;
    else
        this->_hash[idx] = hash;
    return false;
}

void HashTable::save(std::string filename)
{
}

void HashTable::load(std::string filename)
{
}

// EPDBook

EPDBook::EPDBook()
{
    this->_idx = 0;
    this->_fens.clear();
    this->_fens.push_back(Board::get_start_fen());
}

void EPDBook::load(std::string filename, int seed)
{
    if (filename.empty())
        return;

    this->_idx = 0;
    this->_fens.clear();

    std::cout << "Loading book: " << filename << std::endl;

    std::ifstream file(filename);

    std::string fen;
    while (std::getline(file, fen))
        if (!fen.empty())
            this->_fens.push_back(fen);

    if (seed == 0)
        seed = std::time(nullptr);
    auto rng = std::default_random_engine(seed);
    std::shuffle(std::begin(this->_fens), std::end(this->_fens), rng);

    std::cout << this->_fens.size() << " fens loaded and shuffled." << std::endl;
}

std::string EPDBook::get_fen()
{
    std::string res = "";
    this->_lock.lock();
    res = this->_fens[this->_idx];
    this->_idx++;
    if (this->_idx >= this->_fens.size())
        this->_idx = 0;
    this->_lock.unlock();
    return res;
}

// Datagen

static const i64 DG_FILE_SIZE = 10000000;
static const int DG_FILE_POS_LEN = 48;

static const std::vector<std::string> &SYZYGY_PATHES = {
    "/dev/shm/syzygy",
    "/dev/shm/Syzygy",
    "/mnt/chess/Syzygy",
    "/home/freeman/chess/Syzygy",
    "z:\\Syzygy",
    "c:\\Syzygy",
    "d:\\Chess\\Syzygy",
};

DataGen::DataGen()
{
    this->_games.clear();
    this->_nns.clear();

#ifdef USE_PSTREAMS
    this->_engines.clear();
#endif
}

void DataGen::init(int threads_num, int hash1, int hash2, std::string book)
{
    this->_hash.resize(hash2);

    TranspositionTable &table = TranspositionTable::instance();
    if (hash1 == 0)
        table.disable();
    else
        table.init(hash1);

    if (!book.empty())
        this->_book.load(book);

    for (const auto &path: SYZYGY_PATHES)
        if (Syzygy::instance().init(path))
            break;

    this->_games.resize(threads_num);
}

#ifdef USE_PSTREAMS
void DataGen::set_enemy(std::string path_to_engine, int depth, int time, int nodes, int hash, bool use_syzygy)
{
    this->_enemy_depth = depth;
    this->_enemy_time = time;
    this->_enemy_nodes = nodes;

    std::string syzygy_path = Syzygy::instance().get_path();

    this->_engines.resize(this->_games.size());

    for (auto &engine: this->_engines)
    {
        engine.open(path_to_engine);
        engine.uci();
        engine.set_hash(hash);
        engine.set_threads(1);
        if (use_syzygy)
            engine.set_syzygy(syzygy_path);
        engine.isready();
    }
}
#endif

void DataGen::gen(std::string out_file, int files, int file_idx)
{
    this->_filename = out_file;

    this->_dg_max_size = DG_FILE_SIZE * files;
    this->_dg_file_idx = file_idx;

    this->_size = 0;
    this->_timer.start();

    this->_dataset_in.resize(2);
    this->_dataset_in[0].resize(DG_FILE_SIZE * DG_FILE_POS_LEN);
    this->_dataset_in[1].resize(DG_FILE_SIZE * DG_FILE_POS_LEN);

    this->_dataset_count.resize(this->_dg_max_size / DG_FILE_SIZE);
    for (auto &item : this->_dataset_count)
        item = 0;

    this->_res_depth = 0;
    this->_res_white = 0;
    this->_res_black = 0;
    this->_res_draw = 0;
    this->_enemy_win = 0;

    std::mt19937 mt(std::time(nullptr));
    mt();   mt();   mt();

    std::vector<std::thread> threads;
    for (int i = 0; i < this->_games.size(); ++i)
        threads.push_back(std::thread(&DataGen::thread_gen, this, i, mt()));

    for (int i = 0; i < this->_games.size(); ++i)
        threads[i].join();
}

void DataGen::thread_gen(int thread_id, unsigned int seed)
{
    auto &game = this->_games[thread_id];
    game._prune_pv_moves_count = false;

#ifdef USE_PSTREAMS
    UciEngine *engine = nullptr;
    if (this->_engines.size() != 0)
        engine = &this->_engines[thread_id];
#endif

    Randomizer rnd(seed);
    int game_count = 0;

    std::vector<DGPos> positions;

    while (true)
    {
        int resign = 0;
        bool no_resign = rnd.random01() > 0.9f;

        int move_start = 0;
        std::string uci_position = "startpos moves";

        bool use_enemy = false;
        int enemy_color = game_count % 2;

        game_count++;

        if (rnd.random01() > 0.05f)
        {
            std::string fen = this->_book.get_fen();
            game.set_fen(fen);
            uci_position = "fen " + fen + " moves";
            move_start = 4;
        }
        else
            game.set_startpos();

        positions.clear();

#ifdef USE_PSTREAMS
        if (engine != nullptr && rnd.random01() > 0.9f)
        {
            use_enemy = true;
            engine->ucinewgame();
            engine->clear_hash();
            engine->isready();
        }
#endif

        int count;
        int game_result = 1;    // 0 - black; 1 - draw; 2 - white;
        int res_prev = 0;
        bool prev_save = false;
        i64 depth_avg = 0;
        i64 depth_cnt = 0;
        int move_num;
        for (move_num = move_start; move_num < 500; ++move_num)
        {
            if (move_num >= 0 && move_num < 4)
                count = 7;
            else if (move_num >= 4 && move_num < 8)
                count = 5;
            else if (move_num >= 8 && move_num < 12)
                count = 3;
            else if (move_num >= 12 && move_num < 16)
                count = 2;
            else
                count = 1;

            if (game._board.check_draw(0))
            {
                game_result = 1;
                break;
            }

            int depth = 8;
            i16 res = game.go_multi(depth, 2800, count, 128, true, &this->_lock3);

            if (prev_save && abs(res) < 700 && abs(res_prev) < 700 && abs(res + res_prev) > 300 && positions.size() > 2)
                positions.pop_back();

            res_prev = res;
            prev_save = false;

            if (!game._board.is_white(0))
                res = -res;;

            if (game._variants.size() == 0)
            {
                if (res > 20000)
                    game_result = 2;
                else if (res < -20000)
                    game_result = 0;
                else game_result = 1;

                if (res == 0)
                {
//                    std::cout << "variants.size == 0" << " -- game_result: " << game_result << " -- res: " << res << std::endl;
//                    game._board.print();
                    game_result = -1;
                }

                break;
            }

            if (!no_resign && abs(res) >= 1000)
            {
                resign++;
                if (resign > 5)
                {
                    if (res > 0)
                        game_result = 2;
                    else
                        game_result = 0;
                    break;
                }
            }
            else
                resign = 0;

            if (res == 19999)
            {
                game_result = 2;
                break;
            }
            else if (res == -19999)
            {
                game_result = 0;
                break;
            }

            int random = game._variants.size() > 1 ? rnd.random01() * game._variants.size() : 0;
            if (random >= game._variants.size())
                std::cout << "random >= game._variants.size()" << std::endl;

            u16 move = game._variants[random]._move;
            if (move == 0)
            {
                std::cout << "move == 0" << std::endl;
                game_result = -1;
                break;
            }

            if (move_num >= 16)
            {
                if (!this->_hash.check_hash(game._board.get_hash(0)))
                {
                    if (!game._board.is_check(0))
                    {
//                        game._board._nodes[0]._pv[0] = 0;
//                        game.quiescence_tune(0, -20000, 20000);

//                        int pv_size = game._board._nodes[0]._pv[0];
//                        if (pv_size == 0)
                        if ((move & Move::KILLED) == 0 && (((move >> 12) & 7) == 0))
                        {
                            depth_avg += depth;
                            depth_cnt++;

                            i16 eval = game.eval();
                            if (!game._board.is_white(0))
                                eval = -eval;

                            prev_save = true;
                            positions.push_back({0,
                                                 res,
                                                 eval,
                                                 static_cast<u8>(game._board.color(0)),
                                                 game._board._bitboards[0][Board::KING],
                                                 game._board._bitboards[1][Board::KING],
                                                 game._board._bitboards[0][Board::PAWN],
                                                 game._board._bitboards[1][Board::PAWN],
                                                 game._board._bitboards[0][Board::KNIGHT],
                                                 game._board._bitboards[1][Board::KNIGHT],
                                                 game._board._bitboards[0][Board::BISHOP],
                                                 game._board._bitboards[1][Board::BISHOP],
                                                 game._board._bitboards[0][Board::ROOK],
                                                 game._board._bitboards[1][Board::ROOK],
                                                 game._board._bitboards[0][Board::QUEEN],
                                                 game._board._bitboards[1][Board::QUEEN]});
                        }
                    }
                }
            }

#ifdef USE_PSTREAMS
            if (use_enemy && engine != nullptr && move_num >= 16 && (move_num % 2) == enemy_color)
            {
                engine->position(uci_position);
                std::string move_str = engine->go(this->_enemy_depth, this->_enemy_time, this->_enemy_nodes);
                u16 enemy_move = game.get_legal_move(move_str);
                if (enemy_move != 0)
                    move = enemy_move;
                else
                    std::cout << "!!! enemy move is not legal! " << move_str << std::endl;
            }
#endif

            if (!game._board.move_do(move, 0, true))
            {
                std::cout << "!move_do" << std::endl;
                game_result = -1;
                break;
            }
            game._board.add_position();
            game._age = (game._age + 1) % 64;

            uci_position += " " + Move::get_string(move);
        }

        if (depth_cnt == 0 || positions.size() == 0)
            continue;

        if (game_result == 0)
        {
            this->_res_black++;
            if (enemy_color == 1)
                this->_enemy_win++;
        }
        else if (game_result == 1)
        {
            // Skip some draws
            if (rnd.random01() > 0.7f)
                continue;
            this->_res_draw++;
        }
        else if (game_result == 2)
        {
            this->_res_white++;
            if (enemy_color == 0)
                this->_enemy_win++;
        }
        else if (game_result == -1)
            continue;

        this->_res_depth += 10 * depth_avg / depth_cnt;

        for (auto &pos : positions)
        {
            if (!this->add_pos(pos, game_result))
                return;
        }
    }
}

void DataGen::convert(int threads_num, std::string in_file, std::string out_file)
{
#ifdef USE_CNPY
    this->_nns.resize(threads_num);

    this->_filename = out_file;

    auto arr = cnpy::npy_load(in_file);
    auto data = arr.as_vec<u8>();

    std::cout << "Reading file: " << std::endl;

    _positions.resize(DG_FILE_SIZE);

    std::vector<u8> output;
    output.reserve(DG_FILE_SIZE * DG_FILE_POS_LEN);
    std::vector<int> output_idx;

    int counter = 0;
    int counter_out = 0;
    for (int i = 0; i < DG_FILE_SIZE; ++i)
    {
        if ((i + 1) % 1000000 == 0)
            std::cout << i+1 << " / " << DG_FILE_SIZE << std::endl;

        output_idx.push_back(counter_out);

        int res_sign = data[counter++];
        int res1 = data[counter++];
        int res2 = data[counter++];
        int game_res = data[counter++];

        DGPos& pos = _positions[i];
        std::memset(&pos, 0, sizeof(DGPos));

        pos.game_res = game_res & 3;
        pos.res = ((res1 * 256) + res2) * (res_sign ? -1 : 1);
        pos.color = game_res >> 4;

        int counter_bebin = counter;

        Bitboards::bit_set(pos.wk, data[counter++]);
        Bitboards::bit_set(pos.bk, data[counter++]);

        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.wp, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.wn, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.wb, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.wr, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.wq, data[counter++]);

        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.bp, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.bn, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.bb, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.br, data[counter++]);
        for (int j = data[counter++]; j > 0; j--)
            Bitboards::bit_set(pos.bq, data[counter++]);

        for (int j = 0; j < 5; ++j)     // Размер заголовка
        {
            output.push_back(0);
            counter_out++;
        }
        for (int j = counter_bebin; j < counter; ++j)
        {
            output.push_back(data[j]);
            counter_out++;
        }
    }

    std::cout << "Converting with " << threads_num << " threads:" << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < this->_nns.size(); ++i)
        threads.push_back(std::thread(&DataGen::thread_eval, this, i, this->_nns.size()));
    for (int i = 0; i < this->_nns.size(); ++i)
        threads[i].join();

    std::cout << "Saving file: " << std::endl;
    for (int i = 0; i < DG_FILE_SIZE; ++i)
    {
        int pos = output_idx[i];
        output[pos++] = abs(_positions[i].res) / 256;
        output[pos++] = abs(_positions[i].res) % 256;
        output[pos++] = abs(_positions[i].eval) / 256;
        output[pos++] = abs(_positions[i].eval) % 256;
        output[pos++] = (_positions[i].game_res & 3) | (_positions[i].color << 2) | ((_positions[i].res < 0 ? 1 : 0) << 3) | ((_positions[i].eval < 0 ? 1 : 0) << 4);
    }

    cnpy::npy_save(out_file, &output[0], { output.size() });
#endif
}

void DataGen::thread_eval(int thread_id, int size)
{
    for (int i = thread_id; i < _positions.size(); i += size)
    {
        if (((i+1) % 1000000) == 0)
            std::cout << i+1 << " / " << DG_FILE_SIZE << std::endl;

        int eval = _nns[thread_id].predict_i(_positions[i].color,
                                             _positions[i].wk,
                                             _positions[i].bk,
                                             _positions[i].wp,
                                             _positions[i].bp,
                                             _positions[i].wn,
                                             _positions[i].bn,
                                             _positions[i].wb,
                                             _positions[i].bb,
                                             _positions[i].wr,
                                             _positions[i].br,
                                             _positions[i].wq,
                                             _positions[i].bq);
            _positions[i].eval = eval;
    }
}

bool DataGen::add_pos(DGPos &position, i16 game_res)
{
    int result = false;

    this->_lock1.lock();

    int idx = this->_size / DG_FILE_SIZE;
    int idx2 = idx % 2;
    int num = this->_size % DG_FILE_SIZE;

    this->_size++;
    if (this->_size <= this->_dg_max_size)
        result = true;

    if ((this->_size % 500000) == 0)
    {
        TranspositionTable &table = TranspositionTable::instance();
        table.clear();
    }

    if ((this->_size % 100000) == 0)
    {
        u32 current = this->_timer.get();

        int cur_s = current / 1000;
        int cur_h = cur_s / 3600;
        int cur_m = (cur_s / 60) % 60;
        int cur_d = cur_h / 24;
        cur_s %= 60;
        cur_h %= 24;

        float speed = 1000.0f * this->_size / current;
        int est_s = static_cast<int>((this->_dg_max_size - this->_size) / speed);
        int est_h = est_s / 3600;
        int est_m = (est_s / 60) % 60;
        int est_d = est_h / 24;
        est_s %= 60;
        est_h %= 24;

        float percent = std::roundf(10000.0f * this->_size / this->_dg_max_size) / 100.0f;

        int res_white = this->_res_white;
        int res_black = this->_res_black;
        int res_draw = this->_res_draw;
        int res_enemy = this->_enemy_win;
        int res_total = res_white + res_black + res_draw;
        int avg_depth = static_cast<int>(10.0f * this->_res_depth / res_total);
        float draw_percent = std::roundf(10000.0f * res_draw / res_total) / 100.0f;
        float white_score = res_white + res_draw*0.5f;
        float black_score = res_black + res_draw*0.5f;
        float white_percent = std::roundf(10000.0f * white_score / (white_score+black_score)) / 100.0f;
        float enemy_score = res_enemy + res_draw*0.5f;
        float enemy_percent = std::roundf(10000.0f * enemy_score / (white_score+black_score)) / 100.0f;

        std::cout << this->_size << "/" << this->_dg_max_size << " (" << percent << "%)"
                  << " -- speed: " << static_cast<int>(speed)
                  << " -- depth: " << avg_depth / 100.0f
                  << " -- white: " << white_percent << "%"
                  << " -- draws: " << draw_percent << "%"
                  << " -- enemy: " << enemy_percent << "%"
                  << " -- total: " << res_total
                  << " -- time: " << cur_d << "d " << cur_h << ":" << (cur_m < 10 ? "0" : "") << cur_m << ":" << (cur_s < 10 ? "0" : "") << cur_s
                  << " -- left: " << est_d << "d " << est_h << ":" << (est_m < 10 ? "0" : "") << est_m << ":" << (est_s < 10 ? "0" : "") << est_s
                  << std::endl;
    }

    this->_lock1.unlock();

    if (idx >= this->_dg_max_size / DG_FILE_SIZE)
        return result;

    int pos = DG_FILE_POS_LEN * num;
    this->_dataset_in[idx2][pos++] = 0;
    this->_dataset_in[idx2][pos++] = abs(position.res) / 256;
    this->_dataset_in[idx2][pos++] = abs(position.res) % 256;
    this->_dataset_in[idx2][pos++] = abs(position.eval) / 256;
    this->_dataset_in[idx2][pos++] = abs(position.eval) % 256;
    this->_dataset_in[idx2][pos++] = (game_res & 3) | (position.color << 2) | ((position.res < 0 ? 1 : 0) << 3) | ((position.eval < 0 ? 1 : 0) << 4);
    this->_dataset_in[idx2][pos++] = Bitboards::lsb(position.wk);
    this->_dataset_in[idx2][pos++] = Bitboards::lsb(position.bk);
    store(position.wp, idx2, pos);
    store(position.wn, idx2, pos);
    store(position.wb, idx2, pos);
    store(position.wr, idx2, pos);
    store(position.wq, idx2, pos);
    store(position.bp, idx2, pos);
    store(position.bn, idx2, pos);
    store(position.bb, idx2, pos);
    store(position.br, idx2, pos);
    store(position.bq, idx2, pos);

    int len = pos - DG_FILE_POS_LEN * num;
    if (len > DG_FILE_POS_LEN)
        std::cout << "ALARM!! len: " << len << " pos: " << pos << " start: " << DG_FILE_POS_LEN * num << " idx: " << idx << " idx2: " << idx2 << " num: " << num << std::endl;

    this->_dataset_in[idx2][DG_FILE_POS_LEN * num] = len;

    this->_lock2.lock();
    this->_dataset_count[idx]++;
    int count = this->_dataset_count[idx];
    this->_lock2.unlock();

    if (count == DG_FILE_SIZE)
    {
#ifdef USE_CNPY
        std::vector<u8> output;
        output.reserve(DG_FILE_SIZE * DG_FILE_POS_LEN);

        std::vector<int> shfl;
        for (int i = 0; i < DG_FILE_SIZE; ++i)
            shfl.push_back(i);

        auto rng = std::default_random_engine(std::time(nullptr));
        std::shuffle(std::begin(shfl), std::end(shfl), rng);

        for (int i = 0; i < DG_FILE_SIZE; ++i)
        {
            int pos = DG_FILE_POS_LEN * shfl[i];
            int len = this->_dataset_in[idx2][pos];
            for (int idx = 1; idx < len; ++idx)
                output.push_back(this->_dataset_in[idx2][pos + idx]);
        }
        cnpy::npy_save(this->_filename + "_" + std::to_string(this->_dg_file_idx+idx) +".npy", &output[0], { output.size() });
#endif
    }

    return result;
}

void DataGen::store(u64 bitboard, int idx2, int &pos)
{
    this->_dataset_in[idx2][pos++] = Bitboards::bits_count(bitboard);
    while (bitboard != 0)
        this->_dataset_in[idx2][pos++] = Bitboards::poplsb(bitboard);
}

// BookGen

BookGen::BookGen(int threads)
{
    this->_games.clear();
    this->_games.resize(threads);
}

void BookGen::gen(std::string filename, int hash1, int hash2, int book_depth, int depth, int eval_from, int eval_to)
{
    this->_fens.clear();
    this->_hash.resize(hash2);

    Board board;
    fens_for_book(0, book_depth, board);

    std::cout << "Fens: " << this->_fens.size() << std::endl;

    TranspositionTable &table = TranspositionTable::instance();
    table.init(hash1);

    std::vector<std::thread> threads;
    for (int i = 0; i < this->_games.size(); ++i)
        threads.push_back(std::thread(&BookGen::thread, this, i, depth, eval_from, eval_to));

    for (int i = 0; i < this->_games.size(); ++i)
        threads[i].join();

    std::cout << "Fens out: " << this->_fens_out.size() << std::endl;
    std::ofstream file(filename, std::ios_base::out);
    for (auto &fen : this->_fens_out)
        file << fen << std::endl;
    file.close();
}

void BookGen::thread(int thread_id, int depth, int eval_from, int eval_to)
{
    auto &game = this->_games[thread_id];
    game._prune_pv_moves_count = false;

    std::string fen = "";
    while (!(fen = this->get_fen()).empty())
    {
        game.set_fen(fen);
        int res = game.go_multi(depth, 0, 1, 0);
        if (res < eval_from || res > eval_to)
            continue;

        this->_lock2.lock();
        this->_fens_out.push_back(fen);
        this->_lock2.unlock();
    }
}

std::string BookGen::get_fen()
{
    std::string res = "";

    this->_lock1.lock();
    if (!this->_fens.empty())
    {
        res = this->_fens.back();
        this->_fens.pop_back();
        int size = this->_fens.size();
        if ((size % 10000) == 0)
            std::cout << size << std::endl;
    }
    this->_lock1.unlock();

    return res;
}

void BookGen::fens_for_book(int ply, int depth, Board &board)
{
    if (this->_hash.check_hash(board.get_hash(ply)))
        return;
    if (depth == 0)
    {
        this->_fens.push_back(board.get_fen(ply));
        return;
    }
    auto moves = board.moves_init(ply);
    u16 move = 0;
    while ((move = moves->get_next()) != 0)
    {
        if (!board.move_do(move, ply))
            continue;
        fens_for_book(ply+1, depth-1, board);
        board.move_undo(move, ply);
    }
    board.moves_free();
}

// Duel

#ifdef USE_PSTREAMS

Duel::Duel(UCI *uci)
{
    this->_uci = uci;
}

void Duel::open_engine(std::string engine_cmd, int hash, int threads, std::string syzygy_path)
{
    this->_engine.open(engine_cmd);
    this->_engine.uci();
    this->_engine.set_hash(hash);
    this->_engine.set_threads(threads);
    this->_engine.set_syzygy(syzygy_path);
    this->_engine.isready();
}

void Duel::fight(int games, int self_time, int enemy_time, int inc, int depth, std::string book_file, int book_seed)
{
    EPDBook book;
    book.load(book_file, book_seed);

    for (int i = 0; i < games; ++i)
    {
        std::string fen = book.get_fen();
        this->play(fen, self_time, enemy_time, inc, depth, true);
        this->play(fen, enemy_time, self_time, inc, depth, false);
    }
}

void Duel::print_results()
{
}

int Duel::play(std::string fen, int wtime, int btime, int inc, int depth, bool self_w)
{
    Game game;
    game.set_fen(fen);

    this->_uci->newgame();
    this->_uci->set_fen(fen);

    _engine.ucinewgame();
    _engine.isready();

    std::string position = "fen " + fen;

    int result = 0;

    for (int i = 0; i < 1024; ++i)
    {
        std::cout << i << ". wtime: " << wtime/1000.0 << " -- btime: " << btime/1000.0 << std::endl;
        std::string best_move = "";
        int score = 0;
        int time = 0;

        if (game._board.is_white(0) == self_w)
        {
            Rules rules;
            rules._wtime = wtime;
            rules._btime = btime;
            rules._winc = inc;
            rules._binc = inc;
            rules._depth = depth;

            Timer timer;
            this->_uci->go(rules, false);
            time = timer.get();

            best_move = this->_uci->get_bestmove();
            score = this->_uci->get_score();

            std::cout << "ural:   best_move: " << best_move << " -- score: " << score << " -- time: " << time << std::endl;
        }
        else
        {
            _engine.position(position);
            Timer timer;
            best_move = _engine.go(depth, 0, 0, wtime, btime, inc);
            time = timer.get();
            score = _engine.get_score();

            std::cout << "engine: best_move: " << best_move << " -- score: " << score << " -- time: " << time << std::endl;
        }

        if (game._board.is_white(0))
            wtime = wtime + inc - time;
        else
            btime = btime + inc - time;

        if (!game._board.is_white(0))
            score = -score;

        if (i == 0)
            position += " moves";
        position += " " + best_move;

        if (best_move != "y1z8")
        {
            game.make_move(best_move);
            this->_uci->make_move(best_move);
        }
        else
        {
            std::cout << "!!! y1z8 !!!" << std::endl;
            break;
        }

        u16 dtz_move = 0;
        if (game.egtb_dtz(dtz_move))
        {
            score = game._best_value;
            if (!game._board.is_white(0))
                score = -score;

            if (score > 0)
                result = 1;
            else if (score < 0)
                result = -1;
            else // score == 0
                result = 0;

            break;
        }

        if (game._board.check_draw(0, 3))
        {
            result = 0;
            break;
        }

        if (score == 19999)
        {
            result = 1;
            break;
        }
        else if (score == -19999)
        {
            result = -1;
            break;
        }
    }

    std::cout << "position " << position << std::endl << std::endl;

    return result;
}

#endif
