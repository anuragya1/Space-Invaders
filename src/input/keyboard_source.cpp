// keyboard_source.cpp
#include "keyboard_source.h"
#include "../core/action.h"

namespace si {

std::uint8_t KeyboardSource::poll(std::uint32_t, const Game&, int) {
    std::uint8_t m = 0;
    if (inp_.left   .load()) { m |= action::LEFT;    inp_.left   = false; }
    if (inp_.right  .load()) { m |= action::RIGHT;   inp_.right  = false; }
    if (inp_.shoot  .load()) { m |= action::SHOOT;   inp_.shoot  = false; }
    if (inp_.pause  .load()) { m |= action::PAUSE;   inp_.pause  = false; }
    if (inp_.quit   .load()) { m |= action::QUIT;    inp_.quit   = false; }
    if (inp_.console.load()) { m |= action::CONSOLE; inp_.console = false; }
    return m;
}

} // namespace si
