#include "replay_file.h"

#include <fstream>
#include <sstream>

namespace si {

bool replay_save(const std::string& path, const Replay& rp) {
    std::ofstream f(path);
    if (!f) return false;
    f << "HEADER seed=" << rp.seed
      << " diff="       << rp.diffIdx
      << " mode="       << rp.modeStr
      << " player="     << rp.player;
    if (rp.expectedScore >= 0) f << " score=" << rp.expectedScore;
    if (rp.expectedLevel >= 0) f << " level=" << rp.expectedLevel;
    f << '\n';

    std::size_t i = 0, N = rp.frames.size();
    while (i < N) {
        std::size_t j = i + 1;
        while (j < N
               && rp.frames[j].p1   == rp.frames[i].p1
               && rp.frames[j].p2   == rp.frames[i].p2
               && rp.frames[j].tick == rp.frames[i].tick + (std::uint32_t)(j - i)) {
            ++j;
        }
        std::size_t run = j - i;
        if (run >= 3) {
            f << "RUN " << run << ' '
              << (int)rp.frames[i].p1 << ' ' << (int)rp.frames[i].p2
              << ' ' << rp.frames[i].tick << '\n';
        } else {
            for (std::size_t k = i; k < j; ++k)
                f << rp.frames[k].tick << ' '
                  << (int)rp.frames[k].p1 << ' '
                  << (int)rp.frames[k].p2 << '\n';
        }
        i = j;
    }
    return true;
}

bool replay_load(const std::string& path, Replay& rp) {
    std::ifstream f(path);
    if (!f) return false;
    std::string tok;
    f >> tok;

    std::string line;
    std::getline(f, line);
    std::istringstream is(line);
    while (is >> tok) {
        if      (tok.rfind("seed=",   0) == 0) rp.seed    = (std::uint32_t)std::stoul(tok.substr(5));
        else if (tok.rfind("diff=",   0) == 0) rp.diffIdx = std::stoi(tok.substr(5));
        else if (tok.rfind("mode=",   0) == 0) rp.modeStr = tok.substr(5);
        else if (tok.rfind("player=", 0) == 0) rp.player  = tok.substr(7);
        else if (tok.rfind("score=",  0) == 0) rp.expectedScore = std::stoi(tok.substr(6));
        else if (tok.rfind("level=",  0) == 0) rp.expectedLevel = std::stoi(tok.substr(6));
    }

    std::string body_line;
    while (std::getline(f, body_line)) {
        if (body_line.empty()) continue;
        std::istringstream bs(body_line);
        std::string head;
        bs >> head;
        if (head == "RUN") {
            std::size_t n; int p1, p2; std::uint32_t start;
            if (!(bs >> n >> p1 >> p2 >> start)) continue;
            for (std::size_t k = 0; k < n; ++k) {
                InputFrame fr;
                fr.tick = start + (std::uint32_t)k;
                fr.p1   = (std::uint8_t)p1;
                fr.p2   = (std::uint8_t)p2;
                rp.frames.push_back(fr);
            }
        } else {
            try {
                std::uint32_t t = (std::uint32_t)std::stoul(head);
                int p1, p2;
                if (!(bs >> p1 >> p2)) continue;
                InputFrame fr;
                fr.tick = t;
                fr.p1   = (std::uint8_t)p1;
                fr.p2   = (std::uint8_t)p2;
                rp.frames.push_back(fr);
            } catch (...) { /* skip malformed line */ }
        }
    }
    return true;
}

}
