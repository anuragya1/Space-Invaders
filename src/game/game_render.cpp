// game_render.cpp - draw the world via RBuf, then the HUD lines.
#include "game.h"
#include "../core/colors.h"
#include "../ui/ui_options.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace si {

void Game::render() {
    rbuf_.clear();

    for (const auto& s : stars) rbuf_.set(s.x, s.y, s.sym, color::BLUE);

    // Shields
    for (const auto& sh : shields)
        for (int r = 0; r < 2; ++r)
            for (int c = 0; c < 4; ++c) {
                char cell = sh.cells[r][c];
                if (cell == ' ') continue;
                const char* col = (cell == '#') ? color::GREEN
                                : (cell == '+') ? color::YELLOW : color::RED;
                rbuf_.set(sh.x + c, sh.y + r, cell, col);
            }

    // UFO
    if (ufo.active) {
        rbuf_.set(ufo.x - 1, UFO_Y, '<', color::BMAGENTA);
        rbuf_.set(ufo.x,     UFO_Y, '@', color::BMAGENTA);
        rbuf_.set(ufo.x + 1, UFO_Y, '>', color::BMAGENTA);
    }

    // Aliens
    bool cb = ui::opts().colorblind;
    for (auto& a : aliens)
        if (a.alive) {
            a.frame = animF_;
            char glyph = a.sym();
            if (cb) {
                // distinct shape per row, independent of color
                if      (a.row == 0) glyph = (a.frame == 0) ? '#' : '%';
                else if (a.row == 1) glyph = (a.frame == 0) ? '8' : 'B';
                else                  glyph = (a.frame == 0) ? 'o' : '*';
            }
            rbuf_.set(a.pos.x, a.pos.y, glyph, a.col());
        }

    // Boss
    if (boss.active) {
        const char* bc = boss.col();
        for (int dx = -2; dx <= 2; ++dx) rbuf_.set(boss.x + dx, boss.y,     '#', bc);
        for (int dx = -2; dx <= 2; ++dx) rbuf_.set(boss.x + dx, boss.y + 1, 'V', bc);
        int bar = (boss.hp * 20) / std::max(1, boss.maxHp);
        for (int i = 0; i < 20; ++i)
            rbuf_.set(W / 2 - 10 + i, 1, (i < bar) ? '=' : ' ',
                     (boss.hp < boss.maxHp / 3) ? color::BRED :
                     (boss.hp < 2 * boss.maxHp / 3) ? color::BYELLOW : color::BGREEN);
    }

    // Bullets
    for (const auto& b : bullets)
        if (b.active) rbuf_.set(b.pos.x, b.pos.y, b.sym(), b.col());

    // Power-ups
    for (const auto& p : powerups)
        if (p.active) rbuf_.set(p.pos.x, p.pos.y, p.sym(), p.col());

    // Explosions
    for (const auto& e : explosions)
        rbuf_.set(e.pos.x, e.pos.y, '*', color::BYELLOW);

    // P1
    if (player.lives > 0) {
        const char* pc = player.shielded ? color::BCYAN : color::BWHITE;
        rbuf_.set(player.pos.x, player.pos.y, '^', pc);
        if (player.shielded) {
            rbuf_.set(player.pos.x - 1, player.pos.y, '(', color::CYAN);
            rbuf_.set(player.pos.x + 1, player.pos.y, ')', color::CYAN);
        }
    }
    // P2
    if (hasP2 && player2.lives > 0) {
        const char* pc = player2.shielded ? color::BCYAN : color::BMAGENTA;
        rbuf_.set(player2.pos.x, player2.pos.y, 'A', pc);
        if (player2.shielded) {
            rbuf_.set(player2.pos.x - 1, player2.pos.y, '(', color::CYAN);
            rbuf_.set(player2.pos.x + 1, player2.pos.y, ')', color::CYAN);
        }
    }

    rbuf_.print();

    // ---- HUD line ----
    std::cout << color::BOLD << color::BWHITE
              << " [" << diff_.name << "]" << color::RST
              << color::BYELLOW << "  Sc:" << color::BWHITE
              << std::setw(6) << player.score << color::RST;
    if (hasP2)
        std::cout << color::BMAGENTA << "  P2:"
                  << std::setw(6) << player2.score << color::RST;
    std::cout << color::BRED << "  P1L:";
    for (int i = 0; i < player.lives; ++i) std::cout << "^ ";
    std::cout << color::RST;
    if (hasP2) {
        std::cout << color::BMAGENTA << " P2L:";
        for (int i = 0; i < player2.lives; ++i) std::cout << "A ";
        std::cout << color::RST;
    }
    std::cout << color::BCYAN << "  Lv:" << level_ << color::RST;
    if (combo_ >= 2)
        std::cout << color::BMAGENTA << "  CMB x" << combo_ << color::RST;
    if (mode == Mode::AI_DEMO)     std::cout << color::BBLUE   << "  [AI]"     << color::RST;
    if (mode == Mode::REPLAY)      std::cout << color::BYELLOW << "  [REPLAY]" << color::RST;
    if (mode == Mode::COOP_HOST)   std::cout << color::BGREEN  << "  [HOST]"   << color::RST;
    if (mode == Mode::COOP_CLIENT) std::cout << color::BGREEN  << "  [CLIENT]" << color::RST;
    std::cout << '\n';

    if (flashT_ > 0)
        std::cout << color::BGREEN << color::BOLD << ' ' << flashMsg_
                  << "                                  " << color::RST << '\n';
    else
        std::cout << color::BLUE
                  << " [A/D]Move [SP]Shoot [P]Pause [Q]Quit "
                  << "[~]Console | T=Triple S=Shield R=Rapid\n"
                  << color::RST;
}

} // namespace si
