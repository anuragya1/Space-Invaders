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

}
