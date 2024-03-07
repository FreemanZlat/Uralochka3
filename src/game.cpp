#include "game.h"

#include "hash.h"
#include "uci.h"
#include "syzygy.h"

#include <iostream>
#include <map>
#include <cmath>
#include <memory.h>

static const std::map<char, int> &MORPH_PIECES = { {'q', Board::QUEEN},
                                                   {'r', Board::ROOK},
                                                   {'b', Board::BISHOP},
                                                   {'n', Board::KNIGHT} };

int FUTILITY_PRUNING_HISTORY[] = { 0, 0 };

int COUNTER_PRUNING_DEPTH[] = { 0, 0 };
int COUNTER_PRUNING_HISTORY[] = { 0, 0 };

double SEE_KILL = 0;
double SEE_QUIET = 0;

double LMR_MOVES_0_0 = 0;
double LMR_MOVES_0_1 = 0;
double LMR_MOVES_1_0 = 0;
double LMR_MOVES_1_1 = 0;

double LMR_DEPTH_0 = 0;
double LMR_DEPTH_1 = 0;

int ASPIRATION_DELTA = 0;
double ASPIRATION_DELTA_INC = 0;
double ASPIRATION_ALPHA_SHIFT = 0;
double ASPIRATION_BETA_SHIFT = 0;

int BETA_DEPTH = 0;
double BETA_PRUNING = 0;
int ALPHA_PRUNING = 0;

int NULL_REDUCTION = 0;
int PROBCUT_DEPTH = 0;
int PROBCUT_BETA = 0;

int FUT_MARGIN_0 = 0;
int FUT_MARGIN_1 = 0;
int FUT_MARGIN_2 = 0;

int SINGULAR_DEPTH_1 = 0;
int SINGULAR_DEPTH_2 = 0;
double SINGULAR_COEFF = 0;

int HISTORY_REDUCTION = 0;

static const int TIME_MARGIN = 50;

int TIME_MID = 0;
float TIME_MID_VAL = 0;
float TIME_INC_COEF_MIN = 0;
float TIME_INC_DIV_MIN = 0;
float TIME_INC_COEF_MAX = 0;
float TIME_INC_DIV_MAX = 0;


Rules::Rules():
    _depth(0),
    _movetime(0),
    _wtime(0),
    _btime(0),
    _winc(0),
    _binc(0),
    _movestogo(0),
    _mate(0),
    _infinite(false),
    _nodes(0),
    _ponder(false)
{
}

Game::Game()
{
    this->_depth_max = 64;
    this->_time_min = 0;
    this->_time_mid = 0;
    this->_time_max = 0;
    this->_is_main = false;
    this->_print_uci = false;

    this->_depth_time = 10;

    this->_depth_prev = 0;
    this->_depth_best = 0;
    this->_best_depth = 0;
    this->_best_value = 0;

    this->_eval.init(&this->_board);

    this->_prune_pv_moves_count = true;

    this->lmr_init();
}

void Game::lmr_init()
{
    for (int i = 0; i < 9; ++i)
    {
        this->lmr_moves[0][i] = static_cast<int>(LMR_MOVES_0_0 + LMR_MOVES_0_1 * i * i / 4.5);
        this->lmr_moves[1][i] = static_cast<int>(LMR_MOVES_1_0 + LMR_MOVES_1_1 * i * i / 4.5);
    }
    for (int i = 0; i < 64; ++i)
        for (int j = 0; j < 64; ++j)
        {
            if (i==0 || j==0)
                this->lmr_depth[i][j] = 0;
            else
                this->lmr_depth[i][j] = static_cast<int>(LMR_DEPTH_0 + std::log(static_cast<double>(i)) * std::log(static_cast<double>(j)) / LMR_DEPTH_1);
        }
}

void Game::set_uci(UCI *uci)
{
    this->_uci = uci;
}

void Game::set_960(bool is960)
{
    this->_board._is960 = is960;
}

void Game::set_startpos()
{
    this->_board.reset();
    this->_age = 0;
}

void Game::set_fen(const std::string &fen)
{
    this->_board.set_fen(fen);
    this->_age = 0;
}

u16 Game::get_legal_move(const std::string &move_str)
{
    int from = (move_str[1] - '1')*8 + (move_str[0] - 'a');
    int to = (move_str[3] - '1')*8 + (move_str[2] - 'a');
    int morph = move_str.size() == 5 ? MORPH_PIECES.at(move_str[4]) : 0;
    return this->get_legal_move(from, to, morph);
}

u16 Game::get_legal_move(int from, int to, int morph)
{
    auto moves = this->_board.moves_init(0);
    u16 move = 0;
    while ((move = moves->get_next()) != 0)
    {
        int move_from = move & 63;
        int move_to = (move >> 6) & 63;
        int move_morph = (move >> 12) & 7;
        // Если ход, совпадает с тем, что нужно выполнить - выполняем и выходим
        if (move_from == from && move_to == to)
        {
            if (morph != 0 && move_morph != morph)
                continue;
            this->_board.moves_free();
            return move;
        }
    }
    this->_board.moves_free();
    return 0;
}

void Game::make_move(const std::string &move_str)
{
    // Парсим ход
    u16 move = this->get_legal_move(move_str);
    if (move == 0)
    {
        std::cout << "info string Illegal move: " << move_str << std::endl;
        return;
    }

    this->_board.move_do(move, 0, true);
    this->_board.add_position();
    this->_age = (this->_age + 1) % 64;
}

u64 Game::go_bench(int depth)
{
    this->_nodes = 0;

    u16 move = 0;
    this->search_root(depth, move);

    return this->_nodes;
}

int Game::go_multi(int &depth, int nodes, int count, int threshold, bool chech_egtb, std::mutex *mutex)
{
    this->_variants.clear();

    if (chech_egtb && mutex)
    {
        u16 move = 0;

        mutex->lock();

        Syzygy &egtb = Syzygy::instance();
        auto egtb_probe = egtb.probe_dtz(
                this->_board._all_pieces[0],
                this->_board._all_pieces[1],
                this->_board._bitboards[0][Board::KING] | this->_board._bitboards[1][Board::KING],
                this->_board._bitboards[0][Board::QUEEN] | this->_board._bitboards[1][Board::QUEEN],
                this->_board._bitboards[0][Board::ROOK] | this->_board._bitboards[1][Board::ROOK],
                this->_board._bitboards[0][Board::BISHOP] | this->_board._bitboards[1][Board::BISHOP],
                this->_board._bitboards[0][Board::KNIGHT] | this->_board._bitboards[1][Board::KNIGHT],
                this->_board._bitboards[0][Board::PAWN] | this->_board._bitboards[1][Board::PAWN],
                this->_board.is_white(0),
                move
            );

        mutex->unlock();

        if (egtb_probe != Syzygy::SYZYGY_NONE && move != 0)
        {
            int score = 0;
            if (egtb_probe == Syzygy::SYZYGY_WIN)
                score = 20000 - 2;
            else if (egtb_probe == Syzygy::SYZYGY_LOSE)
                score = -20000 + 2;

            int move_from = move & 63;
            int move_to = (move >> 6) & 63;
            int move_morph = (move >> 12) & 7;
            move = this->get_legal_move(move_from, move_to, move_morph);

            this->_variants.push_back({ move, score });
            return score;
        }
    }

    this->_nodes = 0;
    int best = -30000;

    if (count > 1)
    {
        for (int i = 0; i < count; ++i)
        {
            u16 move = 0;
            int res = this->search_root(depth, move);

            if (res > best)
                best = res;

            if (move == 0)
                return best;

            if (res + threshold >= best)
                this->_variants.push_back({ move, res });
        }
    }
    else
    {
        u16 move = 0;
        int current_depth = 3;
        best = this->search_root(current_depth++, move);
        while (true)
        {
            move = 0;
            if (std::abs(best) > 1500)
                best = this->search_root(current_depth, move);
            else
                best = this->search_aspiration(current_depth, best, move);

            if (move == 0)
                return best;

            if ((current_depth >= depth*3) || (current_depth >= depth && this->_nodes >= nodes))
            {
                depth = current_depth;
                this->_variants.push_back({ move, best });
                break;
            }

            current_depth++;
        }
    }

    return best;
}

void Game::go_rules(Rules rules, bool print_uci)
{
    // Настраиваем учёт установленных параметров
    this->_is_main = true;
    this->_print_uci = print_uci;
    this->rules_parser(rules);
    this->go();
}

void Game::go()
{
    // Засекаем таймер
    this->_timer.start();

#ifdef IS_TUNING
    this->lmr_init();
#endif

    this->_best_depth = 0;
    this->_best_move = "y1z8";
    this->_sel_depth = 0;
    this->_nodes = 0;
    this->_tbhits = 0;
    u16 best_move = 0;
    int res = this->_best_value;
    int prev_best = this->_best_value;
    int prev_move = 0;

//    if (this->_time_min != 0 && this->_print_uci)
//        std::cout << "info string time_min = " << this->_time_min / 1000.0 << "  time_max = " << this->_time_max / 1000.0 << std::endl;

    int depth_prev = 4 * (this->_depth_prev / 4);

    // Постепенно перебираем всё глубже, начиная с глубины 1
    for (int i = depth_prev > 0 ? 4 : 1; i <= this->_depth_max; i += i < depth_prev ? 4 : 1)
    {
        prev_move = best_move;

        // Синхронизируем глубину поиска с другими потоками
        if (i > 1)
        {
            int max_i = this->_depth_best + 1;
            if (i < max_i)
                i = max_i;
        }

        // Запускаем поиск с заданной глубиной
        if (std::abs(res) > 1500)
            res = this->search_root(i, best_move);
        else
            res = this->search_aspiration(i, res, best_move);

        // Если отмена перебора - выходим из цикла и юзаем предыдущий лучший ход
        if (this->_is_cancel)
            break;

        this->set_bestmove(i, best_move, res);

        // Выиграли или проиграли
        if (res < -19000 || res > 19000)
            break;

        if (this->_is_main && i > 1)
        {
            this->_uci->sync(this->_timer.get());

            // Проверяем оставшееся время
            if (i >= this->_depth_time)
            {
                auto time_current = this->_timer.get();
                int time = this->_time_min;
                if (best_move != prev_move || prev_best - res > TIME_MID)
                {
//                    std::cout << "info string using time_mid = " << this->_time_mid / 1000.0 << std::endl;
                    time = this->_time_mid;
                }
                // Если израсходовали более половины заданного времени, то перебирать на следующую глубину нет смысла
                if (time > 0 && time_current >= time)
                    break;
            }
        }
    }
}

void Game::stop()
{
    this->_is_cancel = true;
}

bool Game::egtb_dtz(u16 &best_move)
{
    Syzygy &egtb = Syzygy::instance();
    auto egtb_probe = egtb.probe_dtz(
            this->_board._all_pieces[0],
            this->_board._all_pieces[1],
            this->_board._bitboards[0][Board::KING] | this->_board._bitboards[1][Board::KING],
            this->_board._bitboards[0][Board::QUEEN] | this->_board._bitboards[1][Board::QUEEN],
            this->_board._bitboards[0][Board::ROOK] | this->_board._bitboards[1][Board::ROOK],
            this->_board._bitboards[0][Board::BISHOP] | this->_board._bitboards[1][Board::BISHOP],
            this->_board._bitboards[0][Board::KNIGHT] | this->_board._bitboards[1][Board::KNIGHT],
            this->_board._bitboards[0][Board::PAWN] | this->_board._bitboards[1][Board::PAWN],
            this->_board.is_white(0),
            best_move
        );

    if (egtb_probe == Syzygy::SYZYGY_NONE || best_move == 0)
        return false;

    int score = 0;
    if (egtb_probe == Syzygy::SYZYGY_WIN)
        score = 20000 - 7;
    else if (egtb_probe == Syzygy::SYZYGY_LOSE)
        score = -20000 + 7;

    this->set_bestmove(1, best_move, score);

    if (this->_print_uci)
        this->_uci->info(1, 1, 0, score, UCI::EXACT, Move::get_string(best_move));

    return true;
}

int Game::eval()
{
    return this->_eval.eval(0, -20000, 20000);
}

void Game::rules_parser(Rules &rules)
{
    // Глубина перебора
    this->_depth_max = 64;
    if (rules._depth > 0)
        this->_depth_max = rules._depth;

    // Оставшееся время и инкремент за наш цвет
    this->_time_min = 0;
    this->_time_max = 0;
    if (rules._movetime > 0)
    {
        this->_time_max = rules._movetime - TIME_MARGIN;
        if (this->_time_max < 1)
            this->_time_max  = 1;
        this->_time_min = this->_time_max;
        this->_time_mid = this->_time_max;
    }

    int time = rules._wtime;
    int time_inc = rules._winc;
    if (!this->_board.is_white(0))
    {
        time = rules._btime;
        time_inc = rules._binc;
    }

//    if (this->_print_uci)
//        std::cout << "info string time: " << time << " -- inc: " << time_inc << std::endl;

    if (time > 0)
    {
        int time_margin = std::max(1, time - TIME_MARGIN);

        if (rules._movestogo == 0)
        {
            this->_time_min = -TIME_MARGIN + std::round((TIME_INC_COEF_MIN*time_inc + time) / TIME_INC_DIV_MIN);
            this->_time_max = -TIME_MARGIN + std::round((TIME_INC_COEF_MAX*time_inc + time) / TIME_INC_DIV_MAX);
        }
        else
        {
            // Нужно тюнить или пофиг? =)
            this->_time_min = (time + time_inc*rules._movestogo) * 2 / ((rules._movestogo + 1) * 3);
            this->_time_max = 9 * this->_time_min / 4;
        }

        this->_time_min = std::max(1, this->_time_min);
        this->_time_min = std::min(time_margin, this->_time_min);

        this->_time_max = std::max(1, this->_time_max);
        this->_time_max = std::min(time_margin, this->_time_max);

        this->_time_mid = std::round((1.0 - TIME_MID_VAL) * this->_time_min + TIME_MID_VAL * this->_time_max);
        this->_time_mid = std::max(1, this->_time_mid);
        this->_time_mid = std::min(time_margin, this->_time_mid);
    }
}

int Game::search_aspiration(int depth, int previous_result, u16 &best_move)
{
    int res = previous_result;
    // Размер окна
    int delta = ASPIRATION_DELTA;
    // Начальное окно
    int alpha = depth > 5 ? res - delta : -20000;
    int beta = depth > 5 ? res + delta : 20000;
    if (alpha < -20000)
        alpha = -20000;
    if (beta > 20000)
        beta = 20000;

    while (true)
    {
        // Ищем с данным окном
        u16 move_best = 0;
        res = this->search(depth, 0, alpha, beta, move_best, 0, false);

        // Если поиск прерван - выходим из цикла
        if (this->_is_cancel)
            break;

        // Если попали в окно
        if (res > alpha && res < beta)
        {
            best_move = move_best;
            this->set_bestmove(depth, best_move, res);
            if (this->_print_uci)
                this->_uci->info(depth, this->_sel_depth, this->_timer.get(), res, UCI::EXACT, this->_board._nodes[0].get_pv());
            break;
        }

        // Если меньше окна
        if (res <= alpha)
        {
            if (this->_print_uci)
                this->_uci->info(depth, this->_sel_depth, this->_timer.get(), res, UCI::UPPERBOUND, this->_board._nodes[0].get_pv());
            beta = std::round((1.0 - ASPIRATION_BETA_SHIFT) * alpha + ASPIRATION_BETA_SHIFT * beta);
            alpha = alpha - delta;
            if (alpha < -20000)
                alpha = -20000;
        }
        // Если больше окна
        else if (res >= beta)
        {
            best_move = move_best;
            this->set_bestmove(depth, best_move, res);
            if (this->_print_uci)
                this->_uci->info(depth, this->_sel_depth, this->_timer.get(), res, UCI::LOWERBOUND, this->_board._nodes[0].get_pv());
            alpha = std::round(ASPIRATION_ALPHA_SHIFT * alpha + (1.0 - ASPIRATION_ALPHA_SHIFT) * beta);
            beta = beta + delta;
            if (beta > 20000)
                beta = 20000;
        }

        // Увеличиваем размер окна
        delta = ASPIRATION_DELTA_INC * delta;
    }

    return res;
}

int Game::search_root(int depth, u16 &best_move)
{
    // Увеличиваем счётчик перебранных позиций
    this->_nodes++;

    u64 hash = this->_board.get_hash(0);

    // Берём значения из хэш-таблицы
    TranspositionTable &table = TranspositionTable::instance();
    TTNode node;
    bool hash_hit = table.load(hash, node);

    // Ход из хэш-таблицы
    u16 hash_move = 0;
    if (hash_hit)
        hash_move = node._move;

    // Текущая оценка позиции. По возможности из хэш-таблицы
    int eval = hash_hit ? node._eval : this->_eval.eval(0, -20000, 20000);
    this->_board._nodes[0]._eval = eval;

    // Если шах - увеличиваем глубину перебора
    bool is_check = this->_board.is_check(0);
    int search_depth = depth;
    if (is_check)
        search_depth++;

    // Генерим ходы
    auto moves = this->_board.moves_init(0);
    moves->update_hash(hash_move);

    this->_board._nodes[0]._pv[0] = 0;

    u64 output_nodes = 0;

    // Поиск лучшего хода
    int best = -20000;
    int legal_moves = 0;
    u16 move = 0;
    TTNode::Type hash_type = TTNode::ALPHA;
    u16 move_best = 0;
    while ((move = moves->get_next()) != 0)
    {
        // При multipv пропускаем ранее рассмотренные ходы
        bool skip = false;
        for (auto &variant : this->_variants)
            if (move == variant._move)
                skip = true;

        // Если ход нелегальный, смотрим следующий ход
        if (skip || !this->_board.move_do(move, 0))
            continue;

        legal_moves++;
        if (this->_print_uci && this->_nodes > 1000000 && this->_nodes - output_nodes > 500000)
        {
            output_nodes = this->_nodes;
            this->_uci->info(depth, legal_moves, Move::get_string(move));
        }

        // PV-Search
        int res = 0;
        if (legal_moves > 1)
            res = -search(search_depth-1, 1, -best-1, -best, move_best, 0, false);

        if (legal_moves == 1 || res > best)
            res = -search(search_depth-1, 1, -20000, -best, move_best, 0, false);

        // Отматываем ход
        this->_board.move_undo(move, 0);

        // Если отмена перебора - выходим
        if (this->_is_cancel)
        {
            this->_board.moves_free();
            return best;
        }

        // Если результат поиска лучше того, что мы уже нашли
        if (res > best)
        {
            this->_board._nodes[0]._pv[0] = this->_board._nodes[1]._pv[0] + 1;
            this->_board._nodes[0]._pv[1] = move;
            memcpy(&this->_board._nodes[0]._pv[2], &this->_board._nodes[1]._pv[1], this->_board._nodes[1]._pv[0] * sizeof(u16));

            if (this->_print_uci)
                this->_uci->info(depth, this->_sel_depth, this->_timer.get(), res, UCI::EXACT, this->_board._nodes[0].get_pv());

            best = res;
            hash_type = TTNode::EXACT;
            best_move = move;
            this->set_bestmove(depth, best_move, res);
        }
    }

    this->_board.moves_free();

    // Если не было легальных ходов
    if (legal_moves == 0)
    {
        // Если мы не под шахом - пат
        if (!is_check)
            best = 0;
    }

    // Сохраняем результат в хэш-таблицу
    if (best_move != 0)
        table.save(hash, depth, best, eval, hash_type, best_move, this->_age);

    return best;
}

int Game::search(int depth, int ply, int alpha, int beta, u16 &best_move, int skip_move, bool cut_node)
{
    this->_board._nodes[ply]._pv[0] = 0;

    // Увеличиваем счётчик перебранных позиций
    this->_nodes++;
    if (ply > this->_sel_depth)
        this->_sel_depth = ply;

    // Если просрочили время, выходим
    if (this->check_time())
        return beta;

    // Если добрались до глубины 0 - запускаем оценочную функцию
    if (depth <= 0)
        return quiescence(ply, alpha, beta);

    if (ply > 0)
    {
        // Проверка повторений позиции. Нужно бы добавить проверку ничьи по материалу
        if (this->_board.check_draw(ply))
            return 1 - (this->_nodes & 2);

        if (ply >= 125)
            return this->_eval.eval(ply, alpha, beta);

        int r_alpha = alpha > -20000 + ply ? alpha : -20000 + ply;
        int r_beta = beta < 20000 - ply - 1 ? beta : 20000 - ply - 1;
        if (r_alpha >= r_beta)
            return r_alpha;
    }

    bool is_pv_node = (beta - alpha) != 1;

    // Берём значения из хэш-таблицы
    u64 hash = this->_board.get_hash(ply);
    TranspositionTable &table = TranspositionTable::instance();
    TTNode node;
    bool hash_hit = table.load(hash, node);

    // Проверяем значение из хэш-таблицы
    u16 hash_move = 0;
    if (skip_move == 0 && hash_hit)
    {
        if (node._depth >= depth && !is_pv_node && node._value > -19000 && node._value < 19000)
        {
            // Отсечения по значению из хэш-таблицы
            if (node._age_type == TTNode::EXACT)
                return node._value;
            if (node._age_type == TTNode::ALPHA && node._value <= alpha)
                return node._value;
            if (node._age_type == TTNode::BETA && node._value >= beta)
                return node._value;
        }

        // Ход из хэш-таблицы
        hash_move = node._move;
    }

    // Если не в корне, смотрим в ЭБ
    if (ply > 0)
    {
        Syzygy &egtb = Syzygy::instance();
        auto egtb_probe = egtb.probe_wdl(
                this->_board._all_pieces[0],
                this->_board._all_pieces[1],
                this->_board._bitboards[0][Board::KING] | this->_board._bitboards[1][Board::KING],
                this->_board._bitboards[0][Board::QUEEN] | this->_board._bitboards[1][Board::QUEEN],
                this->_board._bitboards[0][Board::ROOK] | this->_board._bitboards[1][Board::ROOK],
                this->_board._bitboards[0][Board::BISHOP] | this->_board._bitboards[1][Board::BISHOP],
                this->_board._bitboards[0][Board::KNIGHT] | this->_board._bitboards[1][Board::KNIGHT],
                this->_board._bitboards[0][Board::PAWN] | this->_board._bitboards[1][Board::PAWN],
                this->_board.is_white(ply),
                depth
            );
        if (egtb_probe != Syzygy::SYZYGY_NONE)
        {
            int res = 0;
            int pieces = Bitboards::bits_count(this->_board._all_pieces[0] | this->_board._all_pieces[1]);
            this->_tbhits++;
            TTNode::Type hash_type = TTNode::EXACT;
            if (egtb_probe == Syzygy::SYZYGY_WIN)
            {
                res = 20000 - 64 - ply - pieces;
                hash_type = TTNode::BETA;
            }
            if (egtb_probe == Syzygy::SYZYGY_LOSE)
            {
                res = -20000 + 64 + ply + pieces;
                hash_type = TTNode::ALPHA;
            }
            
            if ((hash_type == TTNode::EXACT) || (hash_type == TTNode::ALPHA ? (res <= alpha) : (res >= beta)))
            {
                table.save(hash, depth, res, res, hash_type, 0, this->_age);
                return res;
            }
        }
    }

    bool is_check = this->_board.is_check(ply);

    // Текущая оценка позиции. По возможности из хэш-таблицы
    int eval = 0;
    if (hash_hit)
    {
        eval = node._eval;
/*
        // Не работает :(
        if (!is_check && node._value > -19000 && node._value < 19000)
        {
            if ((node._age_type == TTNode::EXACT) ||
                (node._age_type == TTNode::BETA && node._value > eval) ||
                (node._age_type == TTNode::ALPHA && node._value < eval))
            {
                eval = node._value;
            }
        }
*/
    }
    else
    {
        eval = this->_eval.eval(ply, alpha, beta);
        if (!is_check)
            table.save(hash, 0, eval, eval, TTNode::NONE, 0, this->_age);
    }

    this->_board._nodes[ply]._eval = eval;

    // Есть ли улучшение оценки по сравнению с тем, что было 2 полухода назад
    int improving = 0;
    if (ply >= 2 && !is_check && eval > this->_board._nodes[ply-2]._eval)
        improving = 1;

    if (!is_pv_node && !is_check)
    {
        // Razoring
        // if (depth <= 1 && eval + 150 < alpha)
        //     return this->quiescence(ply, alpha, beta);

        // Бета-отсечения
        if (depth <= BETA_DEPTH && eval - std::round(BETA_PRUNING*(depth - improving)) >= beta)
            return eval;

        // Альфа-отсечения
        // if (depth <= 5 && eval + ALPHA_PRUNING <= alpha)
        //     return eval;

        // Нулевой ход
        bool can_null = this->_board._nodes[ply-1]._move != 0 && (ply < 2 || this->_board._nodes[ply-2]._move != 0);
        if (can_null && depth >= 2 && beta > -19000 && eval >= beta &&
            (!hash_hit || node._age_type != TTNode::ALPHA || node._value >= beta) &&
            this->_board.is_figures(this->_board.color(ply)))
        {
            // Редукция
            int reduction = (eval - beta) / NULL_REDUCTION;
            if (reduction > 3)
                reduction = 3;
            reduction += 4 + depth / 6;

            this->_board.nullmove_do(ply);
            int res = -search(depth - reduction, ply+1, -beta, -beta+1, best_move, 0, !cut_node);
            this->_board.nullmove_undo(ply);

            if (res >= beta)
                return beta;
        }

        // ProbCut
        int probcut_beta = beta + PROBCUT_BETA;
        if (skip_move == 0 && depth >= (PROBCUT_DEPTH+1) && abs(beta) < 19000)
        {
            auto moves = this->_board.moves_init(ply, true);
            moves->update_hash(hash_move);
            u16 move = 0;
            while ((move = moves->get_next()) != 0)
            {
                if (!this->_board.move_do(move, ply))
                    continue;

                int res = -quiescence(ply+1, -probcut_beta, -probcut_beta+1);
                if (res >= probcut_beta)
                    res = -search(depth-PROBCUT_DEPTH, ply+1, -probcut_beta, -probcut_beta+1, best_move, 0, !cut_node);

                this->_board.move_undo(move, ply);

                if (res >= probcut_beta)
                {
                    this->_board.moves_free();
                    return res;
                }
            }
            this->_board.moves_free();
        }
    }

    // IID - похоже IIR лучше
/*
    if (depth >= 8 && hash_move == 0)
    {
        int iidepth = depth - depth/4 - 5;
        search(iidepth, ply, alpha, beta, best_move, 0, cut_node);
        if (this->_board._nodes[ply]._pv[0] > 0)
        {
            bool hit = table.load(hash, node);
            if (hit)
            {
                hash_move = node._move;
                this->_board._nodes[ply]._pv[0] = 0;
            }
        }
    }
*/
    // IIR
    if (!(skip_move || is_check))
        if (depth >= 4 && hash_move == 0 && (is_pv_node || cut_node))
            depth--;

    // Генерим ходы
    auto moves = this->_board.moves_init(ply);
    moves->update_hash(hash_move);

    int seeMargin[2];
    seeMargin[0] = std::round(SEE_KILL * depth * depth);
    seeMargin[1] = std::round(SEE_QUIET * depth);

    u64 output_nodes = 0;

    // Поиск лучшего хода
    int best = -20000;
    int moves_all = 0;
    int moves_quiets = 0;
    int moves_legal = 0;
    bool skip_quiets = false;
    bool is_quiet = false;
    u16 move = 0;
    u16 move_best = 0;
    TTNode::Type hash_type = TTNode::ALPHA;
    while ((move = moves->get_next(skip_quiets)) != 0)
    {
        moves_all++;
        if (skip_move == move)
            continue;

        // При multipv пропускаем ранее рассмотренные ходы
        if (ply == 0)
        {
            bool skip = false;
            for (auto &variant : this->_variants)
                if (move == variant._move)
                    skip = true;
            if (skip)
                continue;
        }

        int pawn_morph = (move >> 12) & 7;
        bool is_tactical = (move & Move::KILLED) != 0 || (pawn_morph != 0);
        is_quiet = !is_tactical;

        // Истории
        int hist = 0;
        int hist_counter = 0;
        int hist_follower = 0;
        if (is_quiet)
            hist = moves->get_history(this->_board.color(ply), move, this->_board._board[move & 63] & Board::PIECES_MASK, hist_counter, hist_follower);

        bool can_prune = this->_board.is_figures(this->_board.color(ply)) && best > -19000;
        if (can_prune && is_pv_node)
            can_prune = this->_prune_pv_moves_count;

        if (can_prune)
        {
            // Отсечение по количеству просмотренных ходов
            if (depth <= 8 && moves_all >= this->lmr_moves[improving][std::min(8, depth)])
                skip_quiets = true;


            // Отсекаем тихие ходы (как в Ethereal)
            if (is_quiet)
            {
                int lmr_depth = std::max(0, depth - this->lmr_depth[std::min(63, depth)][std::min(63, moves_legal)]);
                int fut_margin = FUT_MARGIN_0 + lmr_depth * FUT_MARGIN_1;

                if (!is_check && lmr_depth <= 8 && eval + fut_margin <= alpha && hist < FUTILITY_PRUNING_HISTORY[improving])
                    skip_quiets = true;

                if (!is_check && lmr_depth <= 8 && eval + fut_margin + FUT_MARGIN_2 <= alpha)
                    skip_quiets = true;

                if (!moves->is_killer(move) && moves_all > 1 &&
                    lmr_depth <= COUNTER_PRUNING_DEPTH[improving] &&
                    std::min(hist_counter, hist_follower) < COUNTER_PRUNING_HISTORY[improving])
                        continue;
            }

            // Отсечение по SEE
            if (depth <= 9 && !is_check)
                if (moves->SEE(move) < seeMargin[is_quiet ? 1 : 0])
                    continue;
        }
        // Если считаем тихие ходы до проверки на легальность
        if (is_quiet)
        {
            moves->_hist_moves.push_back(static_cast<int>(move) | ((this->_board._board[move & 63] & Board::PIECES_MASK) << 16));
            moves_quiets++;
        }

        int extension = 0;
        // Сингулярное продление
        if (depth >= SINGULAR_DEPTH_1 &&
            skip_move == 0 &&
            hash_move == move &&
            ply != 0 &&
            node._value > -19000 && node._value < 19000 &&
            node._age_type == TTNode::BETA &&
            node._depth >= depth - SINGULAR_DEPTH_2)
        {
            int beta_cut = node._value - depth;
            int res = search(std::round(SINGULAR_COEFF * (depth-1)), ply, beta_cut - 1, beta_cut, best_move, move, cut_node);

            if (res < beta_cut)
                extension = 1;
            else if (beta_cut >= beta)
            {
                this->_board.moves_free();
                return beta_cut;
            }
           else if (node._value >= beta)
                extension = (is_pv_node ? 1 : 0) - 2;
           else if (cut_node)
               extension = -2;
           else if (node._value <= res)
               extension = -1;
        }
        // Если шах - увеличиваем глубину перебора
        if (is_check || pawn_morph != 0)
            extension = 1;

        int search_depth = depth + extension;

        // Если ход нелегальный, смотрим следующий ход
        if (!this->_board.move_do(move, ply))
            continue;
        moves_legal++;

        // Выводим спам в консольку
        if (this->_print_uci && ply == 0 && this->_nodes > 1000000 && this->_nodes - output_nodes > 500000)
        {
            output_nodes = this->_nodes;
            this->_uci->info(depth, moves_legal, Move::get_string(move));
        }

        int res = 0;

        // Сокращения глубины для тихих ходов (LMR). Жесткая тема, но здорово усиливает игру :)
        int reduction = 1;
        if (skip_move == 0 && is_quiet && depth > 2 && moves_legal > 1)
        {
            reduction = this->lmr_depth[std::min(63, depth)][std::min(63, moves_legal)];
            if (is_pv_node)
                reduction--;
            else
                reduction++;
            if (!improving)
                reduction++;
            if (moves->is_killer(move))
                reduction--;
            if (is_check && (this->_board._nodes[ply]._move_piece & Board::PIECES_MASK) == Board::KING)
                reduction++;
            if ((hash_move & Move::KILLED) != 0)
                reduction++;
            if (cut_node)
                reduction += 2;
            reduction -= std::max(-2, std::min(2, hist/HISTORY_REDUCTION));
            reduction = std::min(depth - 1, std::max(1, reduction));

            // Если есть сокращение глубины
            if (reduction != 1)
            {
                // Выполняем сокращённый поиск
                res = -search(search_depth-reduction, ply+1, -alpha-1, -alpha, best_move, 0, true);
                // Если нашли хорошее значение - ищем без сокращений
                if (res > alpha)
                    res = -search(search_depth-1, ply+1, -alpha-1, -alpha, best_move, 0, !cut_node);
            }
        }

        // Если не выполняли сокращённый поиск
        // Для не PV-нод или для последующих ходов в PV-ноде
        if (reduction == 1 && (!is_pv_node || moves_legal != 1))
                // Поиск с нулевым окном
                res = -search(search_depth-1, ply+1, -alpha-1, -alpha, best_move, 0, !cut_node);

        // В PV-ноде
        // Если первый ход или при хорошем результате поиска с нулевым окном при последующих ходах
        if (is_pv_node && (moves_legal == 1 || res > alpha))
                // Поиск с полным окном
                res = -search(search_depth-1, ply+1, -beta, -alpha, best_move, 0, false);

        // Отматываем ход
        this->_board.move_undo(move, ply);

        // Если отмена перебора - выходим
        if (this->_is_cancel)
        {
            this->_board.moves_free();
            return best;
        }

        // Если результат поиска лучше того, что мы уже нашли
        if (res > best)
        {
            this->_board._nodes[ply]._pv[0] = this->_board._nodes[ply+1]._pv[0] + 1;
            this->_board._nodes[ply]._pv[1] = move;
            memcpy(&this->_board._nodes[ply]._pv[2], &this->_board._nodes[ply+1]._pv[1], this->_board._nodes[ply+1]._pv[0] * sizeof(u16));
            best = res;
            move_best = move;
        }

        // Альфа-бета отсечения :)
        if (res > alpha)
        {
            hash_type = TTNode::EXACT;
            alpha = res;
        }
        if (alpha >= beta)
            break;
    }

    this->_board.moves_free();

    if (alpha >= beta)
    {
        hash_type = TTNode::BETA;
        if (is_quiet)
        {
            // Апдейтим киллеры
            moves->update_killers(move);
            // Апдейтим историю
            moves->update_history(depth, this->_board.color(ply));
        }
    }

    // Если не было легальных ходов
    if (moves_legal == 0)
    {
        // Если мы под шахом - нам мат :(
        if (is_check || skip_move != 0)
            best += ply;
        // Если нет - пат (ничья)
        else
            best = 0;
    }

    // Сохраняем результат в хэш-таблицу
    if (skip_move == 0)
        table.save(hash, depth, best, eval, hash_type, move_best, this->_age);

    if (ply == 0)
        best_move = move_best;

    return best;
}

int Game::quiescence(int ply, int alpha, int beta)
{
    this->_board._nodes[ply]._pv[0] = 0;

    this->_nodes++;
    if (ply > this->_sel_depth)
        this->_sel_depth = ply;
    // Если просрочили время, выходим
    if (this->check_time())
        return beta;

    if (ply >= 125)
        return this->_eval.eval(ply, alpha, beta);

    // Проверка повторений позиции. Нужно бы добавить проверку ничьи по материалу
    if (this->_board.check_draw(ply))
        return 1 - (this->_nodes & 2);

    // Значение из хэш-таблицы
    u64 hash = this->_board.get_hash(ply);
    TranspositionTable &table = TranspositionTable::instance();
    TTNode node;
    bool hash_hit = table.load(hash, node);

    u16 hash_move = 0;
    if (hash_hit)
    {
        // Отсечение значением из хэш-таблицы
        if (node._value > -19000 && node._value < 19000)
        {
            if (node._age_type == TTNode::EXACT)
                return node._value;
            if (node._age_type == TTNode::ALPHA && node._value <= alpha)
                return node._value;
            if (node._age_type == TTNode::BETA && node._value >= beta)
                return node._value;
        }

        // Ход из хэш-таблицы
//        hash_move = node._move;
    }

    bool is_check = this->_board.is_check(ply);

    // Текущая оценка позиции. По возможности из хэш-таблицы
    int eval = 0;
    if (hash_hit)
    {
        eval = node._eval;
/*
        // Не работает :(
        if (!is_check && node._value > -19000 && node._value < 19000)
        {
            if ((node._age_type == TTNode::EXACT) ||
                (node._age_type == TTNode::BETA && node._value > eval) ||
                (node._age_type == TTNode::ALPHA && node._value < eval))
            {
                eval = node._value;
            }
        }
*/
    }
    else
    {
        eval = this->_eval.eval(ply, alpha, beta);
        if (!is_check)
            table.save(hash, 0, eval, eval, TTNode::NONE, 0, this->_age);
    }

    int best = -20000 + ply;
    if (!is_check)
    {
        best = eval;
        if (best >= beta)
            return best;
        // Повышаем альфу оценкой
        if (best > alpha)
            alpha = best;
    }

    // Генерим взятия. Если шах, генерим все ходы
    auto moves = this->_board.moves_init(ply, !is_check);

    // Что-то с этим не то. Что не помню :(
//    moves->update_hash(hash_move);

    // Просматриваем все ходы
    u16 move = 0;
    u16 move_best = 0;
    TTNode::Type hash_type = TTNode::ALPHA;
    while ((move = moves->get_next()) != 0)
    {
        if (!this->_board.move_do(move, ply))
            continue;

        int res = -this->quiescence(ply+1, -beta, -alpha);

        this->_board.move_undo(move, ply);

        if (this->_is_cancel)
        {
            this->_board.moves_free();
            return alpha;
        }

        if (res > best)
        {
            this->_board._nodes[ply]._pv[0] = this->_board._nodes[ply+1]._pv[0] + 1;
            this->_board._nodes[ply]._pv[1] = move;
            memcpy(&this->_board._nodes[ply]._pv[2], &this->_board._nodes[ply+1]._pv[1], this->_board._nodes[ply+1]._pv[0] * sizeof(u16));
            move_best = move;
            best = res;
        }
        if (res > alpha)
        {
            hash_type = TTNode::EXACT;
            alpha = res;
        }
        if (alpha >= beta)
        {
            hash_type = TTNode::BETA;
            break;
        }
    }

    this->_board.moves_free();

    // Сохраняем в хэш-таблицу
    table.save(hash, 0, best, eval, hash_type, move_best, this->_age);

    return best;
}

int Game::quiescence_tune(int ply, int alpha, int beta)
{
    if (ply >= 125)
        return this->_eval.eval(ply, alpha, beta);

    int eval = this->_eval.eval(ply, alpha, beta);
    if (eval >= beta)
        return eval;

    if (eval > alpha)
        alpha = eval;

    if (std::max(150, this->_board.get_max_figure_value(this->_board.color(ply))) < alpha - eval)
        return eval;

    this->_board._nodes[ply+1]._pv[0] = 0;

    bool is_check = this->_board.is_check(ply);
    auto moves = this->_board.moves_init(ply, !is_check);

    // Просматриваем все ходы
    u16 move = 0;
    while ((move = moves->get_next()) != 0)
    {
        if (!this->_board.move_do(move, ply))
            continue;

        int res = -this->quiescence_tune(ply+1, -beta, -alpha);

        this->_board.move_undo(move, ply);

        if (res > alpha)
        {
            this->_board._nodes[ply]._pv[0] = this->_board._nodes[ply+1]._pv[0] + 1;
            this->_board._nodes[ply]._pv[1] = move;
            memcpy(&this->_board._nodes[ply]._pv[2], &this->_board._nodes[ply+1]._pv[1], this->_board._nodes[ply+1]._pv[0] * sizeof(u16));
            alpha = res;
        }
        if (alpha >= beta)
            break;
    }

    this->_board.moves_free();

    return alpha;
}

bool Game::check_time()
{
    // Каждые 1000 позиций
    if ((this->_nodes % 1000) > 0)
        return false;

    // Проверяем оставшееся время
    auto time_current = this->_timer.get();
    if (this->_time_max > 0 && time_current >= this->_time_max)
    {
        this->_is_cancel = true;
        return true;
    }

    if (!this->_print_uci || (this->_nodes % 1000000) > 0)
        return false;

    // Выводим дополнительную инфу
    if (time_current > 0)
        this->_uci->info(time_current);

    return false;
}

void Game::set_bestmove(int depth, u16 best_move, int result)
{
    this->_best_depth = depth;
    this->_best_value = result;
    this->_best_move = Move::get_string(best_move);
    this->_best_pv = this->_board._nodes[0].get_pv();
}
