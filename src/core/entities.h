// entities.h - simple POD-ish entity structs.
//
// These are deliberately small and own no resources. The Game class
// holds vectors of these and updates them each tick.
#pragma once

#include "constants.h"
#include "colors.h"
#include <string>

namespace si {

struct Pt {
    int x = 0, y = 0;
    Pt() = default;
    Pt(int x, int y) : x(x), y(y) {}
    bool operator==(const Pt& o) const { return x == o.x && y == o.y; }
};

enum class PUType { NONE, TRIPLE, SHIELD, RAPID };

struct Bullet {
    Pt   pos;
    int  dir;        // -1 up (player) | +1 down (alien)
    bool active = true;
    int  owner;      // 0 = P1, 1 = P2, -1 = alien
    Bullet(int x, int y, int d, int o = 0) : pos(x, y), dir(d), owner(o) {}
    void move() {
        pos.y += dir;
        if (pos.y <= 0 || pos.y >= H - 1) active = false;
    }
    char sym() const { return dir == -1 ? '|' : '!'; }
    const char* col() const {
        if (dir == +1) return color::BRED;
        return (owner == 1) ? color::BMAGENTA : color::BCYAN;
    }
};

struct Alien {
    Pt   pos;
    bool alive = true;
    int  row;
    int  frame = 0;
    Alien(int x, int y, int r) : pos(x, y), row(r) {}
    char sym() const {
        if (row == 0) return frame == 0 ? 'W' : 'V';
        if (row == 1) return frame == 0 ? 'M' : 'W';
        return frame == 0 ? 'V' : 'M';
    }
    const char* col() const {
        if (row == 0) return color::BRED;
        if (row == 1) return color::BYELLOW;
        return color::BGREEN;
    }
    int pts() const { return (3 - row) * 10; }
};

struct Player {
    Pt     pos;
    int    lives;
    int    score      = 0;
    PUType power      = PUType::NONE;
    int    powerTimer = 0;
    bool   shielded   = false;
    int    shieldHP   = 0;
    int    id;
    Player(int l, int pid = 0)
        : pos(W / 2 + (pid == 1 ? 4 : 0), H - 2),
          lives(l), id(pid) {}
    void mvL() { if (pos.x > 1)     --pos.x; }
    void mvR() { if (pos.x < W - 2) ++pos.x; }
};

struct Shield {
    int  x, y;
    char cells[2][4];
    Shield(int sx, int sy) : x(sx), y(sy) {
        for (auto& row : cells) for (auto& c : row) c = '#';
    }
    bool hit(int bx, int by) {
        int lx = bx - x, ly = by - y;
        if (lx < 0 || lx >= 4 || ly < 0 || ly >= 2) return false;
        char& cell = cells[ly][lx];
        if (cell == ' ') return false;
        if      (cell == '#') cell = '+';
        else if (cell == '+') cell = '.';
        else                  cell = ' ';
        return true;
    }
};

struct PowerUp {
    Pt     pos;
    PUType type;
    bool   active = true;
    int    life   = 140;
    PowerUp(int x, int y, PUType t) : pos(x, y), type(t) {}
    char sym() const {
        switch (type) {
            case PUType::TRIPLE: return 'T';
            case PUType::SHIELD: return 'S';
            case PUType::RAPID:  return 'R';
            default:             return '?';
        }
    }
    const char* col() const {
        switch (type) {
            case PUType::TRIPLE: return color::BMAGENTA;
            case PUType::SHIELD: return color::BCYAN;
            case PUType::RAPID:  return color::BYELLOW;
            default:             return color::WHITE;
        }
    }
};

struct UFO {
    int  x      = -1;
    bool active = false;
    int  dir    = 1;
    int  timer  = 0;
};

struct Expl {
    Pt  pos;
    int timer = 6;
    Expl(int x, int y) : pos(x, y) {}
};

struct Star {
    int  x, y;
    char sym;
};

// Multi-stage boss encounter, appears every 5 levels.
struct Boss {
    bool active     = false;
    int  x          = W / 2;
    int  y          = 3;
    int  dir        = 1;
    int  hp         = 0;
    int  maxHp      = 0;
    int  moveTimer  = 0;
    int  shootTimer = 0;
    int  pattern    = 0;   // 0 line, 1 spread, 2 aimed
    int  stage      = 1;   // ramps up as hp drops
    const char* col() const {
        if (stage == 1) return color::BMAGENTA;
        if (stage == 2) return color::BRED;
        return color::BYELLOW;
    }
};

} // namespace si
