// replay_source.h - feeds recorded InputFrames back into the game loop.
#pragma once

#include "input_source.h"
#include <cstddef>
#include <vector>

namespace si {

class ReplaySource : public IInputSource {
public:
    ReplaySource(const std::vector<InputFrame>& frames, int which);
    std::uint8_t poll(std::uint32_t tick, const Game& g, int pid) override;

private:
    const std::vector<InputFrame>& frames_;
    std::size_t                    idx_ = 0;
    int                            which_;
};

} // namespace si
