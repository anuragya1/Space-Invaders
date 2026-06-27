#pragma once

#include <cstdint>
#include <string>

namespace si {

struct Config {
    int           net_port        = 7777;
    std::string   ai_profile      = "balanced";
    std::uint32_t ai_seed         = 0;
    std::string   log_level       = "off";
    std::string   log_file        = "si_pro.log";
    int           default_diff    = 1;
    bool          colorblind      = false;
    bool          sound           = false;
    bool          quick_restart   = true;
    std::string   language        = "en";

    std::string   sdl3_user            = "";
    bool          sdl3_fullscreen      = false;
    bool          sdl3_muted           = false;
    bool          sdl3_director_enabled = true;
    bool          sdl3_reduced_motion  = false;
};

bool load_config(const std::string& path, Config& cfg);

bool save_config(const std::string& path, const Config& cfg);

}
