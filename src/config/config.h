// config.h - simple "key = value" config file loader.
//
// Loads si_pro.cfg from the working directory at startup. Missing file
// is fine - defaults apply. Format:
//
//   # comments start with '#'
//   net.port = 7777
//   ai.profile = balanced     # aggressive | defensive | balanced
//   ai.seed = 0               # 0 = wall-clock
//   log.level = info          # debug | info | warn | error | off
//   log.file = si_pro.log
//   game.default_diff = 1
//
// Unknown keys are silently ignored so older configs still load.
#pragma once

#include <cstdint>
#include <string>

namespace si {

struct Config {
    int           net_port        = 7777;
    std::string   ai_profile      = "balanced";
    std::uint32_t ai_seed         = 0;        // 0 = use time()
    std::string   log_level       = "off";
    std::string   log_file        = "si_pro.log";
    int           default_diff    = 1;
    bool          colorblind      = false;     // use symbol shapes, not just colors
    bool          sound           = false;     // emit terminal BEL on events
    bool          quick_restart   = true;      // R key at game-over re-launches
    std::string   language        = "en";      // en | hi

    // SDL3 build only - terminal build ignores these but preserves
    // their values on rewrite.
    std::string   sdl3_user            = "";       // remembered username
    bool          sdl3_fullscreen      = false;    // start in fullscreen
    bool          sdl3_muted           = false;    // start with sound off
    bool          sdl3_director_enabled = true;    // adaptive difficulty AI
    bool          sdl3_reduced_motion  = false;    // disable screen shake
};

// Load from path; missing file returns defaults. Returns true if file
// was found and parsed (even if empty); false if it didn't exist.
bool load_config(const std::string& path, Config& cfg);

// Write current config to disk. Returns true on success. Format is
// the same key=value text loader format - human-readable, hand-editable.
bool save_config(const std::string& path, const Config& cfg);

} // namespace si
