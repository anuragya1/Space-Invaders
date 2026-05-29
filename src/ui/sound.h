// sound.h - terminal BEL on key events, gated by ui::opts().sound.
//
// We use only the ASCII BEL char (\a, 0x07) - no audio library, no
// platform-specific code. Some terminals make a real sound; some
// flash the window; some do nothing. The point is it's free, optional,
// and never breaks anything.
#pragma once

#include "ui_options.h"

#include <cstdio>

namespace si::ui {

inline void beep() {
    if (!opts().sound) return;
    std::fputc('\a', stdout);
    std::fflush(stdout);
}

} // namespace si::ui
