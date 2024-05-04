#ifndef TRAIN_UTILS_H
#define TRAIN_UTILS_H

#include <vector>
#include <string>
#include <mutex>
#include <atomic>

#include "game.h"
#include "common.h"
#include "uci.h"

#ifdef USE_PSTREAMS
#include "uciengine.h"
#endif

class Tuner
{
public:
    Tuner(int threads);

    enum Type
    {
        TUNE,
        NEURAL,
        DATASET
    };

    void load(std::string filename, Type type, int max_size = -1);
    double eval();
    void eval_thread(int thread_id);
    bool eval_get(int &start, int &size, double res, bool print = false);
    void compute_k();
    void start(std::string in, std::string out);
    void eval_dataset(std::string out_file);
    void dataset_thread(int thread_id);
    void save(std::string out, int iter, double eval);

private:
    i64 _size;
    double _texel_k;
    std::vector<std::string> _fens;
    std::vector<int> _cnt_moves;
    std::vector<double> _results;
    std::vector<int> _books;
    std::vector<int> _evals;

    std::vector<Game> _games;
    double _threads_res;
    int _threads_current;

    std::mutex _lock1;

    std::vector<std::vector<int>> _params;

    Timer _timer;
};

struct HashTable
{
    HashTable();
    ~HashTable();
    void resize(int size_mb);
    bool check_hash(u64 hash);
    void save(std::string filename);
    void load(std::string filename);
    u64* _hash;
    u64 _size;
};

class EPDBook
{
public:
    EPDBook();
    void load(std::string filename, int seed = 0);
    std::string get_fen();

private:
    std::mutex _lock;
    std::vector<std::string> _fens;
    int _idx;
};

struct DGPos
{
    i16 game_res;
    i16 res;
    i16 eval;
    u8 color;
    u64 wk;
    u64 bk;
    u64 wp;
    u64 bp;
    u64 wn;
    u64 bn;
    u64 wb;
    u64 bb;
    u64 wr;
    u64 br;
    u64 wq;
    u64 bq;
};

class DataGen
{
public:
    DataGen(int hash1 = 4096, int hash2 = 4096, std::string book = "");
#ifdef USE_PSTREAMS
    void set_enemy(std::string path_to_engine, int depth = 7, int time = 0, int nodes = 0, int hash = 128, bool use_syzygy = false);
#endif
    void gen(int threads_num, std::string out_file, int files, int file_idx);
    void thread_gen(int thread_id, unsigned int seed);
    void convert(int threads_num, std::string in_file, std::string out_file);
    void thread_eval(int thread_id, int size);
    bool add_pos(DGPos &position, i16 game_res);
    void store(u64 bitboard, int idx2, int &pos);

private:
    i64 _size;

    std::vector<Game> _games;
    std::vector<Neural> _nns;
#ifdef USE_PSTREAMS
    std::vector<UciEngine> _engines;
#endif
    std::vector<DGPos> _positions;


    int _enemy_depth;
    int _enemy_time;
    int _enemy_nodes;

    std::string _filename;

    i64 _dg_max_size;
    int _dg_file_idx;

    std::vector<std::vector<u8>> _dataset_in;
    std::vector<int> _dataset_count;

    Timer _timer;

    EPDBook _book;

    HashTable _hash;

    std::atomic<i64> _res_depth;
    std::atomic<int> _res_white;
    std::atomic<int> _res_black;
    std::atomic<int> _res_draw;
    std::atomic<int> _enemy_win;

    std::mutex _lock1;
    std::mutex _lock2;
    std::mutex _lock3;
};

class BookGen
{
public:
    BookGen(int threads);

    void gen(std::string filename, int book_depth, int depth, int threshold);
    void thread(int thread_id, int depth, int threshold);
    std::string get_fen();
    void fens_for_book(int ply, int depth, Board &board);

private:
    std::vector<Game> _games;

    std::vector<std::string> _fens;
    std::vector<std::string> _fens_out;

    HashTable _hash;

    std::mutex _lock1;
    std::mutex _lock2;
};

#ifdef USE_PSTREAMS
class Duel
{
public:
    Duel(UCI *uci = nullptr);
    void open_engine(std::string engine_cmd, int hash, int threads, std::string syzygy_path);
    void fight(int games, int self_time, int enemy_time, int inc, int depth, std::string book_file, int book_seed);
    void print_results();
    int play(std::string fen, int wtime, int btime, int inc, int depth, bool self_w);

private:
    UciEngine _engine;
    UCI *_uci;

};
#endif

#endif // TRAIN_UTILS_H
