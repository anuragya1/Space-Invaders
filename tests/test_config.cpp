#include "test_common.h"
#include "../src/config/config.h"

#include <cstdio>

int main() {
    using namespace si;

    const char* path = "test_config_tmp.cfg";
    Config out;
    out.sdl3_user = "pilot";
    out.sdl3_fullscreen = true;
    out.sdl3_muted = true;
    out.sdl3_director_enabled = false;
    out.sdl3_reduced_motion = true;

    CHECK(save_config(path, out));

    Config in;
    CHECK(load_config(path, in));
    CHECK_EQ(in.sdl3_user, std::string("pilot"));
    CHECK_EQ(in.sdl3_fullscreen, true);
    CHECK_EQ(in.sdl3_muted, true);
    CHECK_EQ(in.sdl3_director_enabled, false);
    CHECK_EQ(in.sdl3_reduced_motion, true);

    std::remove(path);
    return test_summary("test_config");
}
