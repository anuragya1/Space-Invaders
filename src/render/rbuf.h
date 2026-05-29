// rbuf.h - flicker-free terminal render buffer.
//
// Maintains a W*H grid of (char, color). print() emits ANSI cursor-home
// (\033[H) followed by the frame - so successive frames overwrite the
// previous one without clearing, eliminating flicker.
#pragma once

#include "../core/constants.h"

namespace si {

class RBuf {
public:
    struct Cell { char ch; const char* fg; };

    void clear();
    void set(int x, int y, char c, const char* col);
    void print() const;

private:
    Cell buf_[H][W];
};

} // namespace si
