// replay_source.cpp
#include "replay_source.h"

namespace si {

ReplaySource::ReplaySource(const std::vector<InputFrame>& f, int which)
    : frames_(f), which_(which) {}

std::uint8_t ReplaySource::poll(std::uint32_t tick, const Game&, int) {
    while (idx_ < frames_.size() && frames_[idx_].tick < tick) ++idx_;
    if (idx_ < frames_.size() && frames_[idx_].tick == tick)
        return which_ == 1 ? frames_[idx_].p1 : frames_[idx_].p2;
    return 0;
}

} // namespace si
