// rbuf.cpp
#include "rbuf.h"
#include "../core/colors.h"

#include <iostream>

namespace si {

void RBuf::clear() {
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            if      (x == 0 || x == W - 1) buf_[y][x] = {'|', color::BLUE};
            else if (y == 0 || y == H - 1) buf_[y][x] = {'-', color::BLUE};
            else                            buf_[y][x] = {' ', color::RST};
        }
}

void RBuf::set(int x, int y, char c, const char* col) {
    if (x > 0 && x < W - 1 && y > 0 && y < H - 1) {
        buf_[y][x].ch = c;
        buf_[y][x].fg = col;
    }
}

void RBuf::print() const {
    std::cout << "\033[H";
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const Cell& c = buf_[y][x];
            std::cout << c.fg << c.ch;
        }
        std::cout << color::RST << '\n';
    }
    std::cout.flush();
}

} // namespace si
