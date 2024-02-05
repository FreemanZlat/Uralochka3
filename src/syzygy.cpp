#include "syzygy.h"

#include "bitboards.h"
#include "board.h"

#include <iostream>

#include "fathom/tbprobe.h"

Syzygy &Syzygy::instance()
{
    static Syzygy theSingleInstance;
    return theSingleInstance;
}

bool Syzygy::init(std::string path)
{
    bool res = tb_init(path.c_str());
    std::cout << "info string Syzygy path: '" << path << "' init=" << res << " TB_LARGEST=" << TB_LARGEST << std::endl;

    this->_path = TB_LARGEST > 0 ? path : "";

    return TB_LARGEST > 0;
}

void Syzygy::free()
{
//    tb_free();
}

void Syzygy::set_depth(int depth)
{
    this->_depth = depth;
}

std::string Syzygy::get_path()
{
    return this->_path;
}

Syzygy::Result Syzygy::probe_wdl(u64 wh, u64 bl, u64 k, u64 q, u64 r, u64 b, u64 n, u64 p, bool is_white, int depth)
{
    if (!TB_LARGEST || depth < this->_depth)
        return SYZYGY_NONE;

    int pieces = Bitboards::bits_count(wh | bl);

    if (pieces > TB_LARGEST)
        return SYZYGY_NONE;

    auto res = tb_probe_wdl(wh, bl, k, q, r, b, n, p, 0, 0, 0, is_white);

    if (res == TB_RESULT_FAILED)
        return SYZYGY_NONE;
    else if (res == TB_WIN)
        return SYZYGY_WIN;
    else if (res == TB_LOSS)
        return SYZYGY_LOSE;

    return SYZYGY_DRAW;
}

Syzygy::Result Syzygy::probe_dtz(u64 wh, u64 bl, u64 k, u64 q, u64 r, u64 b, u64 n, u64 p, bool is_white, u16 &best_move)
{
    if (!TB_LARGEST)
        return SYZYGY_NONE;

    int pieces = Bitboards::bits_count(wh | bl);

    if (pieces > TB_LARGEST)
        return SYZYGY_NONE;

    auto res = tb_probe_root(wh, bl, k, q, r, b, n, p, 0, 0, 0, is_white, nullptr);

    if (res == TB_RESULT_FAILED || res == TB_RESULT_CHECKMATE || res == TB_RESULT_STALEMATE)
        return SYZYGY_NONE;

    u16 pawn_morph = 0;
    u16 morph = TB_GET_PROMOTES(res);
    if (morph == TB_PROMOTES_QUEEN)
        pawn_morph = Board::QUEEN;
    if (morph == TB_PROMOTES_ROOK)
        pawn_morph = Board::ROOK;
    if (morph == TB_PROMOTES_BISHOP)
        pawn_morph = Board::BISHOP;
    if (morph == TB_PROMOTES_KNIGHT)
        pawn_morph = Board::KNIGHT;

    best_move = TB_GET_FROM(res) | (TB_GET_TO(res) << 6) | (pawn_morph << 12);

    auto res_wdl = TB_GET_WDL(res);

    if (res_wdl == TB_RESULT_FAILED)
        return SYZYGY_NONE;
    else if (res_wdl == TB_WIN)
        return SYZYGY_WIN;
    else if (res_wdl == TB_LOSS)
        return SYZYGY_LOSE;

    return SYZYGY_DRAW;
}

Syzygy::Syzygy()
{
    this->_depth = 0;
    this->_path = "";
}
