#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "common.h"
#include "evaluate.h"

#include <mutex>

class UCI;

// Правила для хода
struct Rules
{
    int _depth;     // Поиск до нужной глубины
    int _movetime;  // Время на ход (миллисекунды)
    int _wtime;     // Оставшееся время белых (мс)
    int _btime;     // Оставшееся время черных (мс)
    int _winc;      // Инкремент времени белых (мс)
    int _binc;      // Инкремент времени черных (мс)
    int _movestogo; // Сколько ходов до ресета времени
    int _mate;      // Поиск мата в N ходов
    bool _infinite; // Бесконечный поиск
    int _nodes;     // Перебор N позиций
    bool _ponder;   // Думать во время хода противника
    Rules();
};

// Для multipv (пока без pv)
struct Variant
{
    u16 _move;
    int _eval;
};

// Игра
class Game
{
public:
    Game();
    void set_uci(UCI *uci);
    void set_960(bool is960);
    // Установка стартовой позиции
    void set_startpos();
    // Установка позиции из FEN
    void set_fen(const std::string &fen);
    // Проверка легальности хода
    u16 get_legal_move(const std::string &move_str);
    u16 get_legal_move(int from, int to, int morph);
    // Выполнение хода
    void make_move(const std::string &move_str);
    // Думаем над ходом
    u64 go_bench(int depth);
    int go_multi(int &depth, int nodes, int count, int threshold, bool chech_egtb = false, std::mutex *mutex = nullptr);
    void go_rules(Rules rules, bool print_uci = true);
    void go();
    void stop();
    bool egtb_dtz(u16 &best_move);
    int eval();
    int quiescence_tune(int ply, int alpha, int beta);


private:
    // Доска
    Board _board;
    // Возраст
    u8 _age;
    // Таймер
    Timer _timer;
    // Время на ход
    int _time_min;
    int _time_mid;
    int _time_max;
    int _depth_max;
    int _depth_time;
    // Счётчип перебранных позиций
    u64 _nodes;
    u64 _tbhits;
    int _sel_depth;
    // Отмена перебора
    bool _is_cancel;

    bool _is_main;
    bool _print_uci;

    int _depth_prev;
    int _depth_best;
    int _best_depth;
    int _best_value;
    std::string _best_move;
    std::string _best_pv;

    int lmr_moves[2][9];
    int lmr_depth[64][64];
    // Оценщик позиции
    Evaluate _eval;

    std::vector<Variant> _variants;

    UCI *_uci;

    bool _prune_pv_moves_count;

    void lmr_init();
    void rules_parser(Rules &rules);
    // Процедуры поиска
    int search_aspiration(int depth, int previous_result, u16 &best_move);
    int search_root(int depth, u16 &best_move);
    int search(int depth, int ply, int alpha, int beta, u16 &best_move, int skip_move, bool cut_node);
    int quiescence(int ply, int alpha, int beta);
    // Проверка оставшегося времени
    bool check_time();
    void set_bestmove(int depth, u16 best_move, int result);

    friend class UCI;
    friend class Tuner;
    friend class DataGen;
    friend class BookGen;
    friend class Duel;
};

extern int FUTILITY_PRUNING_HISTORY[];

extern int COUNTER_PRUNING_DEPTH[];
extern int COUNTER_PRUNING_HISTORY[];

extern double SEE_KILL;
extern double SEE_QUIET;
extern int SEE_DEPTH;

extern double LMR_MOVES_0_0;
extern double LMR_MOVES_0_1;
extern double LMR_MOVES_1_0;
extern double LMR_MOVES_1_1;

extern double LMR_DEPTH_0;
extern double LMR_DEPTH_1;

extern int ASPIRATION_DELTA;
extern double ASPIRATION_DELTA_INC;
extern double ASPIRATION_ALPHA_SHIFT;
extern double ASPIRATION_BETA_SHIFT;

extern int BETA_DEPTH;
extern double BETA_PRUNING;
extern double BETA_IMPROV_0;
extern double BETA_IMPROV_1;
extern double BETA_HASHHIT_0;
extern double BETA_HASHHIT_1;
extern double BETA_RETURN;

// extern int ALPHA_PRUNING;

extern double NULL_MIN;
extern double NULL_REDUCTION;
extern double NULL_DIV_1;
extern double NULL_DIV_2;

extern int PROBCUT_DEPTH;
extern int PROBCUT_BETA;

extern int IIR_PV_REDUCTION;
extern int IIR_CUT_DEPTH;
extern int IIR_CUT_REDUCTION;

extern int FUT_MARGIN_0;
extern int FUT_MARGIN_1;
extern int FUT_MARGIN_2;

extern int SINGULAR_DEPTH_1;
extern int SINGULAR_DEPTH_2;
extern double SINGULAR_COEFF;

extern int HISTORY_REDUCTION;

extern int TIME_MID;
extern double TIME_MID_VAL;
extern double TIME_INC_COEF_MIN;
extern double TIME_INC_DIV_MIN;
extern double TIME_INC_COEF_MAX;
extern double TIME_INC_DIV_MAX;


#endif // GAME_H
