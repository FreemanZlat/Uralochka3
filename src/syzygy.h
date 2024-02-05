#ifndef SYZYGY_H
#define SYZYGY_H

#include "common.h"

#include <string>

class Syzygy
{
public:
    static Syzygy& instance();

    bool init(std::string path = "");
    void free();
    void set_depth(int depth = 0);
    std::string get_path();

    enum Result: u8
    {
        SYZYGY_NONE = 0,
        SYZYGY_WIN,
        SYZYGY_LOSE,
        SYZYGY_DRAW
    };

    Result probe_wdl(u64 wh, u64 bl, u64 k, u64 q, u64 r, u64 b, u64 n, u64 p, bool is_white, int depth);
    Result probe_dtz(u64 wh, u64 bl, u64 k, u64 q, u64 r, u64 b, u64 n, u64 p, bool is_white, u16 &best_move);

private:
    Syzygy();
    Syzygy(const Syzygy &) = delete;
    Syzygy& operator=(const Syzygy &) = delete;

    int _depth;
    std::string _path;
};

#endif // SYZYGY_H
