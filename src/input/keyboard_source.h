#pragma once

#include "input_source.h"

namespace si {

class KeyboardSource : public IInputSource {
public:
    explicit KeyboardSource(InputState& s) : inp_(s) {}
    std::uint8_t poll(std::uint32_t tick, const Game& g, int pid) override;

private:
    InputState& inp_;
};

}
