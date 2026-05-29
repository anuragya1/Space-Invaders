// coop_source.cpp
#include "coop_source.h"
#include "../core/action.h"
#include "../debug/logger.h"

namespace si {

std::uint8_t CoopSource::poll(std::uint32_t tick, const Game& g, int playerId) {
    if (playerId == self_) {
        std::uint8_t m = kb_.poll(tick, g, playerId);
        if (!sock_.sendAll(&m, 1)) {
            LOG_WARN("send failed at tick " << tick);
            dead_ = true;
            return action::QUIT;
        }
        return m;
    } else {
        std::uint8_t m = 0;
        if (!sock_.recvAll(&m, 1)) {
            LOG_WARN("recv failed at tick " << tick);
            dead_ = true;
            return action::QUIT;
        }
        return m;
    }
}

} // namespace si
