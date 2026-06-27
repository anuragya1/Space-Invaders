/*
    Lockstep input source for LAN co-op.

    The same CoopSource is usually installed for both players. The local
    player's poll sends one action byte; the peer player's poll blocks until
    one action byte arrives. Keep this byte protocol stable.
*/
#pragma once

#include "input_source.h"
#include "../net/tcp_socket.h"

#include <atomic>

namespace si {

class CoopSource : public IInputSource {
public:
    CoopSource(net::TCPSocket& sock, IInputSource& localInput,
               std::atomic<bool>& dead, int self_player)
        : sock_(sock), localInput_(localInput), dead_(dead), self_(self_player) {}

    std::uint8_t poll(std::uint32_t tick, const Game& g, int playerId) override;

private:
    net::TCPSocket&    sock_;
    IInputSource&      localInput_;
    std::atomic<bool>& dead_;
    int                self_;
};

}
