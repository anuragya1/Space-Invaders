// banner.cpp
#include "banner.h"
#include "../core/colors.h"
#include "../core/difficulty.h"
#include "../platform/platform.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace si {

void banner() {
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << color::BCYAN << color::BOLD
              << "\n  +====================================================+\n"
              <<   "  |   " << color::BRED   << " S P A C E   I N V A D E R S "
                              << color::BCYAN << "            |\n"
              <<   "  |   " << color::BWHITE << "    P R O   E D I T I O N    "
                              << color::BCYAN << "            |\n"
              <<   "  |   " << color::DIM    << " AI / Net Co-op / Replay / Editor "
                              << color::BCYAN << "       |\n"
              <<   "  +====================================================+\n"
              << color::RST << '\n';
}

void show_leaderboard() {
    auto lb = leaderboard_read();
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << color::BOLD << color::BWHITE
              << "\n  +==========================================+\n"
              <<   "  |      GLOBAL LEADERBOARD  - TOP 10        |\n"
              <<   "  +====+===============+=======+=============+\n"
              <<   "  | #  | Player        | Score | Mode        |\n"
              <<   "  +----+---------------+-------+-------------+\n" << color::RST;
    if (lb.empty())
        std::cout << "  |       No entries yet.                    |\n";
    for (int i = 0; i < (int)lb.size(); ++i) {
        const char* col = (i == 0) ? color::BYELLOW
                        : (i == 1) ? color::BWHITE
                        : (i == 2) ? color::YELLOW : color::RST;
        std::cout << col << "  | "
                  << std::left << std::setw(3) << (i + 1)
                  << "| " << std::setw(14) << lb[i].name
                  << "| " << std::setw(6)  << lb[i].score
                  << "| " << std::setw(12) << lb[i].diff << "|\n"
                  << color::RST;
    }
    std::cout << color::BWHITE
              << "  +====+===============+=======+=============+\n"
              << color::RST;
    std::cout << "\n  Press ENTER..."; std::cin.get();
}

void show_stats(const Stats& s) {
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << color::BOLD << color::BYELLOW
              << "\n  +==========================================+\n"
              <<   "  |          LIFETIME  STATISTICS            |\n"
              <<   "  +==========================================+\n" << color::RST;
    std::cout << "    Games played   : " << s.gamesPlayed  << '\n'
              << "    Total score    : " << s.totalScore   << '\n'
              << "    Aliens killed  : " << s.aliensKilled << '\n'
              << "    UFOs killed    : " << s.ufosKilled   << '\n'
              << "    Bosses killed  : " << s.bossesKilled << '\n'
              << "    Deaths         : " << s.deaths       << '\n'
              << "    Shots fired    : " << s.shotsFired   << '\n';
    if (s.shotsFired > 0)
        std::cout << "    Accuracy       : "
                  << std::fixed << std::setprecision(1)
                  << (100.0 * s.aliensKilled / s.shotsFired) << "%\n";
    std::cout << "    Power-ups used : " << s.powerupsUsed  << '\n'
              << "    Highest level  : " << s.highestLevel  << '\n'
              << "    Highest combo  : " << s.highestCombo  << '\n';
    std::cout << "\n  Press ENTER..."; std::cin.get();
}

void show_achievements(const std::vector<Achievement>& a) {
    (void)!std::system(SI_CLEAR_CMD);
    std::cout << color::BOLD << color::BMAGENTA
              << "\n  +==========================================+\n"
              <<   "  |             ACHIEVEMENTS                 |\n"
              <<   "  +==========================================+\n" << color::RST;
    int got = 0;
    for (const auto& x : a) {
        if (x.unlocked) {
            std::cout << "  " << color::BGREEN << "[X] " << x.desc << color::RST << '\n';
            ++got;
        } else
            std::cout << "  " << color::DIM << "[ ] " << x.desc << color::RST << '\n';
    }
    std::cout << "\n  Unlocked: " << got << " / " << a.size() << '\n';
    std::cout << "\n  Press ENTER..."; std::cin.get();
}

void show_record(const Record& r) {
    if (r.score == 0) {
        std::cout << "  No record yet for \"" << r.name << "\".\n";
        return;
    }
    std::cout << color::BYELLOW
              << "  +===================================+\n"
              << "  | Player : " << std::left << std::setw(24) << r.name  << "|\n"
              << "  | Best   : "                << std::setw(24) << r.score << "|\n"
              << "  | Level  : "                << std::setw(24) << r.level << "|\n"
              << "  | Mode   : "                << std::setw(24) << r.diff  << "|\n"
              << "  +===================================+\n" << color::RST;
}

void diff_menu() {
    const char* cols[5] = { color::BGREEN, color::BYELLOW, color::YELLOW,
                            color::BRED,   color::BRED };
    std::cout << color::BWHITE
              << "  +---+------------------------+--------------------------+\n"
              << "  | # | Name                   | Tagline                  |\n"
              << "  +---+------------------------+--------------------------+\n"
              << color::RST;
    for (int i = 0; i < N_DIFFS; ++i) {
        const auto& d = difficulty_unchecked(i);
        std::cout << cols[i] << "  |[" << (i + 1) << "]| "
                  << std::left << std::setw(23) << d.name
                  << "| "      << std::setw(25) << d.tag << "|\n" << color::RST;
    }
    std::cout << color::BWHITE
              << "  +---+------------------------+--------------------------+\n\n"
              << color::RST;
}

int pick_difficulty() {
    banner();
    diff_menu();
    std::cout << "  Select difficulty [1-5]: ";
    char dc; std::cin >> dc; std::cin.ignore(10000, '\n');
    int di = dc - '1';
    if (di < 0 || di > N_DIFFS - 1) return -1;
    if (di == 4) {
        std::cout << "\n  " << color::BRED
                  << "!! ULTRA-NIGHTMARE: One life. No saves." << color::RST
                  << "\n  Confirm? [Y/N]: ";
        char yn; std::cin >> yn; std::cin.ignore(10000, '\n');
        if (yn != 'y' && yn != 'Y') return -1;
    }
    return di;
}

} // namespace si
