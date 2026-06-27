#pragma once

#include "ui_options.h"

#include <cstdio>

namespace si::ui {

inline void beep() {
    if (!opts().sound) return;
    std::fputc('\a', stdout);
    std::fflush(stdout);
}

}
