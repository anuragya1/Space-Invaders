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

}
