#ifndef BOARD_H
#define BOARD_H

#include "common.h"
#include "moves.h"

#include "neural.h"

#include <string>
#include <unordered_map>

struct Node
{
    int _flags;
    u64 _hash;
    int _eval;
    u16 _move;
    int _move_piece;
    int _move_killed;
    int _move_en_passant;
    int _extensions;
    u16 _pv[128];

    std::string get_pv(int max = -1);
};

// Класс доски - содержит позицию и разные необходимые функции
// Представление доски - массив 128 интов (8x16)
// Индекс клетки = горизонталь * 16 + вертикаль (начинаются с 0)
// Горизонталь = индекс >> 4; Вертикаль = индекс & 7
// Такое странное представление удобно для быстрой проверки выхода за пределы доски - https://www.chessprogramming.org/0x88
class Board
{
public:
    Board();

    static std::string get_start_fen();

    // Сброс в стартовую позицию
    void reset();

    // Вывод доски в консоль
    void print();

    // Установка позиции в формате FEN - https://www.chessprogramming.org/Forsyth-Edwards_Notation
    void set_fen(const std::string &fen);
    // Получение текущей позиции в формате FEN
    std::string get_fen(int ply);

    // Проверка атакованности поля square фигурами цвета is_white
    bool is_attacked(int square, bool is_white);
    // Дополнительные данные для генератора и сортировки ходов
    Moves* moves_init(int ply, bool kills = false);
    void moves_free();

    // Выполнение хода. Если ход, корректрый, возвращается true. Если некорректный - ход не выполняется и возвращается false
    bool move_do(u16 move, int ply, bool same_ply = false);
    // Отмена хода
    void move_undo(u16 move, int ply);

    void nullmove_do(int ply);
    void nullmove_undo(int ply);

    // Чей ход (true = белые)
    bool is_white(int ply);
    int color(int ply);
    // Проверка шаха ходящей стороны
    bool is_check(int ply);
    // Проверка шаха вражеской стороны
    bool is_enemy_check(int ply);

    // Добавление текущей позиции в список
    void add_position();
    // Проверка повтора позиции
    bool check_draw(int ply, int triple_moves_count = 2);
    // Полуходов до правила 50 ходов
    int get_dtz();

    // Получить хэш
    u64 get_hash(int ply);

    // Вес максимальной фигуры
    int get_max_figure_value(int color);

    // Есть ли фигуры, кроме пешек
    bool is_figures(int color);

    // Константы
    enum pieces
    {
        EMPTY = 0,
        KING,
        QUEEN,
        ROOK,
        BISHOP,
        KNIGHT,
        PAWN,

        PIECES_MASK = 7,
        BLACK_MASK = 8,
        WHITE_MASK = 16,

        FLAG_EN_PASSANT_MASK = 255,
        FLAG_WHITE_MOVE = 256,
        FLAG_WHITE_00 = 512,
        FLAG_WHITE_000 = 1024,
        FLAG_BLACK_00 = 2048,
        FLAG_BLACK_000 = 4096
    };

private:
    void piece_remove(int ply, int color, int piece, int square, bool nn);
    void piece_add(int ply, int color, int piece, int square, bool nn);

    bool _is960;

    // Доска
    int _board[64];

    // Битборды позиции
    u64 _bitboards[2][7];
    u64 _all_pieces[2];

    // Позиции королей
    int _wking;
    int _bking;
    int _wking_area;
    int _bking_area;

    int _start_wrook_00;
    int _start_wrook_000;
    int _start_brook_00;
    int _start_brook_000;

    // Узлы дерева при переборе
    Node _nodes[128];

    // Хэши позиций после ходов до текущей позиции
    std::unordered_map<u64, int> _positions;

    // История beta-отсечений
    History _history;

    // Пул ходов
    int _moves_cnt;
    Moves _moves[256];

    int _dtz_current;
    int _dtz[2048];         // Хватит? ))

    Neural _neural;
    int _stack_pointer;

    friend class Evaluate;
    friend class Game;
    friend class Moves;
    friend class Tuner;
    friend class DataGen;
};

#endif // BOARD_H
