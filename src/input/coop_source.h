// coop_source.h - input source that mediates between local keyboard
// and a remote peer over a TCPSocket, in input-lockstep.
//
// One CoopSource instance is passed as BOTH p1src and p2src to the
// Game loop. On each tick, Game::run calls poll() twice (for P1 then
// P2). The CoopSource switches on playerId vs self_player_:
//   * self call: send our keyboard mask, return it.
//   * peer call: blocking recv 1 byte, return it.
//
// Host gets self_player_=0 (P1). Client gets self_player_=1 (P2).
#pragma once

#include "input_source.h"
#include "keyboard_source.h"
#include "../net/tcp_socket.h"

#include <atomic>

namespace si {

class CoopSource : public IInputSource {
public:
    CoopSource(net::TCPSocket& sock, KeyboardSource& kb,
               std::atomic<bool>& dead, int self_player)
        : sock_(sock), kb_(kb), dead_(dead), self_(self_player) {}

    std::uint8_t poll(std::uint32_t tick, const Game& g, int playerId) override;

private:
    net::TCPSocket&    sock_;
    KeyboardSource&    kb_;
    std::atomic<bool>& dead_;
    int                self_;
};

} // namespace si
