// config.cpp
#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace si {

static std::string trim(std::string s) {
    auto issp = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back()))  s.pop_back();
    return s;
}

static std::string strip_comment(std::string s) {
    auto pos = s.find('#');
    if (pos != std::string::npos) s.erase(pos);
    return s;
}

bool load_config(const std::string& path, Config& cfg) {
    std::ifstream f(path);
    if (!f) return false;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(strip_comment(line));
        if (line.empty()) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key.empty() || val.empty()) continue;

        try {
            if      (key == "net.port")         cfg.net_port      = std::stoi(val);
            else if (key == "ai.profile")       cfg.ai_profile    = val;
            else if (key == "ai.seed")          cfg.ai_seed       = (std::uint32_t)std::stoul(val);
            else if (key == "log.level")        cfg.log_level     = val;
            else if (key == "log.file")         cfg.log_file      = val;
            else if (key == "game.default_diff")cfg.default_diff  = std::stoi(val);
            else if (key == "ui.colorblind")    cfg.colorblind    = (val == "1" || val == "true" || val == "yes");
            else if (key == "ui.sound")         cfg.sound         = (val == "1" || val == "true" || val == "yes");
            else if (key == "ui.quick_restart") cfg.quick_restart = (val == "1" || val == "true" || val == "yes");
            else if (key == "ui.language")      cfg.language      = val;
            else if (key == "sdl3.user")        cfg.sdl3_user        = val;
            else if (key == "sdl3.fullscreen")  cfg.sdl3_fullscreen  = (val == "1" || val == "true" || val == "yes");
            else if (key == "sdl3.muted")       cfg.sdl3_muted       = (val == "1" || val == "true" || val == "yes");
            else if (key == "sdl3.director")    cfg.sdl3_director_enabled = (val == "1" || val == "true" || val == "yes");
            else if (key == "sdl3.reduced_motion") cfg.sdl3_reduced_motion = (val == "1" || val == "true" || val == "yes");
            // unknown keys silently ignored
        } catch (...) { /* malformed value: ignore */ }
    }
    return true;
}

bool save_config(const std::string& path, const Config& cfg) {
    std::ofstream f(path);
    if (!f) return false;
    f << "# si_pro.cfg - written by the in-game Settings menu.\n";
    f << "# Edit by hand or via the Settings screen.\n\n";
    f << "net.port           = " << cfg.net_port      << '\n';
    f << "ai.profile         = " << cfg.ai_profile    << '\n';
    f << "ai.seed            = " << cfg.ai_seed       << '\n';
    f << "log.level          = " << cfg.log_level     << '\n';
    f << "log.file           = " << cfg.log_file      << '\n';
    f << "game.default_diff  = " << cfg.default_diff  << '\n';
    f << "ui.colorblind      = " << (cfg.colorblind    ? "1" : "0") << '\n';
    f << "ui.sound           = " << (cfg.sound         ? "1" : "0") << '\n';
    f << "ui.quick_restart   = " << (cfg.quick_restart ? "1" : "0") << '\n';
    f << "ui.language        = " << cfg.language      << '\n';
    f << "sdl3.user          = " << cfg.sdl3_user        << '\n';
    f << "sdl3.fullscreen    = " << (cfg.sdl3_fullscreen ? "1" : "0") << '\n';
    f << "sdl3.muted         = " << (cfg.sdl3_muted      ? "1" : "0") << '\n';
    f << "sdl3.director      = " << (cfg.sdl3_director_enabled ? "1" : "0") << '\n';
    f << "sdl3.reduced_motion = " << (cfg.sdl3_reduced_motion ? "1" : "0") << '\n';
    return true;
}

} // namespace si
