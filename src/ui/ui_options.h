// ui_options.h - process-wide rendering preferences (colorblind, sound, etc.)
//
// Set once at main() startup from the loaded Config. Read by the
// renderer and the input thread. Plain globals (one writer, many
// readers) - no locking needed.
#pragma once

namespace si::ui {

struct Options {
    bool colorblind = false;
    bool sound      = false;
};

inline Options& opts() {
    static Options o;
    return o;
}

} // namespace si::ui
