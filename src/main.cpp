#include "uci.h"
#include "bitboards.h"
#include "hash.h"
#include "syzygy.h"
#include "neural.h"
#include "tuning_params.h"

#include <iostream>
#include <string>


// Версии:
// 0 - базовая. Оценка - материал + PST. Перебор - альфа-бета + взятия в конце
// 1 - Продление при шахах. Оценка - прикрытие короля и пешечная структура
// 2 - Хэш (пока только для сортировки)
// 3 - Хэш (отсечения), исправление бага превращения пешки
// 4 - Рефакторинг, киллер-ходы, хэш из поиска спокойствия
// 5 - Проверка повторения позиции, сортировка тихих ходов по истории, корневой поиск, исправление контроля времени
// 6 - Генератор взятий. Отсечения по альфе и бете
// 7 - Рефакторинг. Битборды, в т.ч. для генерации ходов (пешки не полностью)
// 8 - Отдельная оценка для мидгейма и эндшпиля. Новые PST
// 9 - Поиск с нулевым окном. Нулевой ход. Узкое окно при старте перебора. Обидный баг в оценке
// 10 - История по 2 предыдущим ходам. Отсечения по количеству просмотренных ходов. Сокращения тихих ходов
// 11 - Futility Prinning
// 12 - Тихие ходы перед move_do
// 13 - Превращения теперь не тихие ходы
// 14 - Киллеры, добавлены условия ничьи, counter-ход
// 15 - Сингулярные продления
// 16 - Многопоточность, отключены сингулярные продления
// 17 - Переработка оценочной функции. Добавлена мобильность фигур. Исправлена редукция в нулевом ходу
// 18 - Вывод PV, доработка оценочной функции (отставшие пешни, фигуры позади пешек, переработана оценка проходных)
// 19 - Сингулярные продления, угрозы королю, защитники короля
// 20 - SEE, delta pruning, фикс багов
// 21 - Переработана хэш-таблица
// 22 - SEE для сортировки ходов, отрицательное сокращение в PV-нодах
// 23 - Исправлен контроль времени
// 24 - Настройка параметров методом движка Texel
// 25 - Убрал pext, защита короля в эндшпиле, ещё раз подбор всех параметров с нуля
// 26 - Простейший бенчмарк, косметические фиксы
// 27 - IID, данные пешечного щита для каждой вертикали, баг в истории, фикс стартового окна
// 28 - Тюнинг оценки с нуля с обновлённым датасетом
// 29 - Исправление ошибок. Коррекция эндшпильной оценки
// 30 - Тюнинг параметров игровой стадии
// 31 - Добавлена поддержка эндшпильных таблиц Syzygy
// 32 - Нейросеть для дополнительной оценки королей и пешек - FAIL :(
// 33 - Нейросеть вместо оценочной функции!
// 34 - Фильтрация позиций при генерации датасета, использование резкльтата партий при обучении. Новая обученная модель
// 35 - Новая обученная модель. Попытка заюзать очерёдность хода в нейронке
// 36 - Новая обученная моедль. Уменьшено количество зон короля в нейросети
// 37 - Новая обученная моедль.
// 38 - Новая обученная моедль. Тюнинг поиска.
// 39 - Исправление зеркалирования позиции в нейросети. Новая обученная моедль. Тюнинг поиска. Новая аллокация хэш-таблицы. Оптимизация сортировки ходов.
// 40 - Новая архитектура сети. Часть датасета из игр против других движков.


#if defined(__AVX512F__)
#define AVX "avx512 "
#elif defined(__AVX2__)
#define AVX "avx2 "
#elif defined(__SSE2__)
#define AVX "sse2 "
#endif

#ifdef USE_PEXT
#define PEXT "pext "
#else
#define PEXT ""
#endif

#ifdef USE_POPCNT
#define POPCNT "popcnt "
#else
#define POPCNT ""
#endif

#ifdef USE_CNPY
#define CNPY "cnpy "
#else
#define CNPY ""
#endif

#ifdef USE_PSTREAMS
#define PSTREAMS "pstreams "
#else
#define PSTREAMS ""
#endif


#define URALOCHKA3 "Uralochka v3.41.dev2"

int main(int argc, char** argv)
{
    std::cout << "info string " << URALOCHKA3 << " " << AVX << PEXT << POPCNT << CNPY << PSTREAMS << std::endl;

    TranspositionTable::instance();
    Model::instance().init();
    Bitboards::instance();
    Syzygy::instance().init();
    TuningParams::instance();

    UCI uci;

    if (argc > 1)
    {
        std::string input = "";
        for(int i = 1; i < argc; ++i)
        {
            if (i > 1)
                input += " ";
            input += argv[i];
        }
        uci.non_uci(input);
    }
    else
        uci.start(URALOCHKA3);

//    Rules rules;
//    rules._depth = 10;
//    uci.set_fen("8/7r/6qP/3r2B1/4p1K1/2p2p2/3n4/2k5 w - - 4 63");
//    uci.go(rules);

//    Tests::goAll();

    return 0;
}
