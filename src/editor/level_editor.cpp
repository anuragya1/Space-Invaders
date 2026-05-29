// level_editor.cpp
#include "level_editor.h"
#include "../core/colors.h"
#include "../core/constants.h"
#include "../persistence/level_file.h"
#include "../platform/platform.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace si {

static void draw_editor(const LevelFile& lv,
                        bool inAliens,
                        int curR, int curC,
                        int shR, int shC) {
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << "\033[H";
    std::cout << color::BOLD << color::BCYAN
              << "  +----------------------------------------------+\n"
              << "  |          LEVEL EDITOR  -  .lvl format        |\n"
              << "  +----------------------------------------------+\n" << color::RST;
    std::cout << "  Name : " << lv.name << '\n';
    std::cout << "  Seed : " << lv.seed << "   Author: " << lv.author << '\n';
    std::cout << "  Timing: move=" << lv.moveDelay
              << "  shoot=" << lv.shootBase
              << "   Boss: " << (lv.boss ? "YES" : "NO") << "\n\n";

    std::cout << "  ALIEN GRID  (" << (inAliens ? "ACTIVE" : "switch with TAB") << "):\n";
    for (int r = 0; r < AROWS; ++r) {
        std::cout << "    ";
        for (int c = 0; c < ACOLS; ++c) {
            bool cur = inAliens && r == curR && c == curC;
            std::cout << (cur ? color::BWHITE : "") << (cur ? color::BOLD : "")
                      << (cur ? "[" : " ") << color::RST;
            if (lv.aliens[r][c])
                std::cout << ((r == 0) ? color::BRED : (r == 1) ? color::BYELLOW : color::BGREEN)
                          << ((r == 0) ? 'W' : (r == 1) ? 'M' : 'V') << color::RST;
            else
                std::cout << color::DIM << '.' << color::RST;
            std::cout << (cur ? color::BWHITE : "") << (cur ? color::BOLD : "")
                      << (cur ? "]" : " ") << color::RST;
        }
        std::cout << '\n';
    }

    std::cout << "\n  SHIELD TEMPLATE  (" << (!inAliens ? "ACTIVE" : "switch with TAB") << "):\n";
    for (int r = 0; r < 2; ++r) {
        std::cout << "    ";
        for (int c = 0; c < 4; ++c) {
            bool cur = !inAliens && r == shR && c == shC;
            std::cout << (cur ? color::BWHITE : "") << (cur ? color::BOLD : "")
                      << (cur ? "[" : " ") << color::RST;
            std::cout << (lv.shield[r][c] ? color::GREEN : color::DIM)
                      << (lv.shield[r][c] ? '#' : '.') << color::RST;
            std::cout << (cur ? color::BWHITE : "") << (cur ? color::BOLD : "")
                      << (cur ? "]" : " ") << color::RST;
        }
        std::cout << '\n';
    }

    std::cout << "\n  " << color::BLUE
              << "[Arrows/WASD] move  [SPC] toggle  [TAB] switch grid  "
              << "[B] boss  [N] name  [V] save  [L] load  [Q] quit\n"
              << color::RST;
}

void run_level_editor(const std::string& user) {
    LevelFile lv;
    int curR = 0, curC = 0;
    int shR  = 0, shC  = 0;
    bool inAliens = true;
    bool dirty    = true;

#ifndef _WIN32
    platform::set_raw_mode(true);
#endif

    while (true) {
        if (dirty) { draw_editor(lv, inAliens, curR, curC, shR, shC); dirty = false; }
        if (!platform::kb_available()) { platform::sleep_ms(20); continue; }
        int ch = platform::read_key();

#ifdef _WIN32
        if (ch == 0 || ch == 224) {
            int c2 = platform::read_key();
            if (c2 == 75) ch = 'a';
            if (c2 == 77) ch = 'd';
            if (c2 == 72) ch = 'w';
            if (c2 == 80) ch = 's';
        }
#else
        if (ch == 27) {
            int c2 = platform::read_key();
            if (c2 == '[') {
                int c3 = platform::read_key();
                if (c3 == 'D') ch = 'a';
                if (c3 == 'C') ch = 'd';
                if (c3 == 'A') ch = 'w';
                if (c3 == 'B') ch = 's';
            }
        }
#endif

        if (ch == 'q' || ch == 'Q') break;
        if (ch == '\t') { inAliens = !inAliens; dirty = true; continue; }
        if (ch == 'b' || ch == 'B') { lv.boss = !lv.boss; dirty = true; continue; }
        if (ch == ' ') {
            if (inAliens) lv.aliens[curR][curC] = !lv.aliens[curR][curC];
            else          lv.shield[shR][shC]   = !lv.shield[shR][shC];
            dirty = true; continue;
        }
        if (inAliens) {
            if      (ch == 'a' || ch == 'A') { if (curC > 0)        --curC; dirty = true; }
            else if (ch == 'd' || ch == 'D') { if (curC < ACOLS-1)  ++curC; dirty = true; }
            else if (ch == 'w' || ch == 'W') { if (curR > 0)        --curR; dirty = true; }
            else if (ch == 's' || ch == 'S') { if (curR < AROWS-1)  ++curR; dirty = true; }
        } else {
            if      (ch == 'a' || ch == 'A') { if (shC > 0) --shC; dirty = true; }
            else if (ch == 'd' || ch == 'D') { if (shC < 3) ++shC; dirty = true; }
            else if (ch == 'w' || ch == 'W') { if (shR > 0) --shR; dirty = true; }
            else if (ch == 's' || ch == 'S') { if (shR < 1) ++shR; dirty = true; }
        }

        // Commands that need cooked-mode input.
        if (ch == 'v' || ch == 'V') {
#ifndef _WIN32
            platform::set_raw_mode(false);
#endif
            std::cout << "\n  Save as (no extension needed): ";
            std::string fn; std::getline(std::cin, fn);
            if (!fn.empty()) {
                if (fn.find('.') == std::string::npos) fn += ".lvl";
                lv.author = user;
                std::cout << (level_save(fn, lv) ? color::BGREEN : color::BRED)
                          << (level_save(fn, lv) ? "  Saved to " : "  Save failed")
                          << fn << '\n' << color::RST;
                platform::sleep_ms(800);
            }
#ifndef _WIN32
            platform::set_raw_mode(true);
#endif
            dirty = true;
        }
        if (ch == 'l' || ch == 'L') {
#ifndef _WIN32
            platform::set_raw_mode(false);
#endif
            std::cout << "\n  Load filename: ";
            std::string fn; std::getline(std::cin, fn);
            if (!fn.empty()) {
                if (fn.find('.') == std::string::npos) fn += ".lvl";
                LevelFile tmp;
                if (level_load(fn, tmp)) { lv = tmp;
                    std::cout << color::BGREEN << "  Loaded.\n" << color::RST;
                } else std::cout << color::BRED << "  Load failed.\n" << color::RST;
                platform::sleep_ms(700);
            }
#ifndef _WIN32
            platform::set_raw_mode(true);
#endif
            dirty = true;
        }
        if (ch == 'n' || ch == 'N') {
#ifndef _WIN32
            platform::set_raw_mode(false);
#endif
            std::cout << "\n  New level name: ";
            std::getline(std::cin, lv.name);
            if (lv.name.empty()) lv.name = "Untitled";
#ifndef _WIN32
            platform::set_raw_mode(true);
#endif
            dirty = true;
        }
    }
#ifndef _WIN32
    platform::set_raw_mode(false);
#endif
}

} // namespace si
