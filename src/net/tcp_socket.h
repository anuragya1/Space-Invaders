// tcp_socket.h - RAII wrapper around a TCP socket.
//
// Move-only. Provides blocking recvAll / sendAll / recvLine / sendLine.
#pragma once

#include "../platform/platform.h"
#include <string>

namespace si::net {

class TCPSocket {
public:
    TCPSocket() = default;
    explicit TCPSocket(socket_t fd) : s_(fd) {}
    ~TCPSocket();

    TCPSocket(const TCPSocket&)            = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;
    TCPSocket(TCPSocket&& o) noexcept;
    TCPSocket& operator=(TCPSocket&& o) noexcept;

    bool valid() const { return s_ != INVALID_SOCK; }

    bool recvAll (void* buf, int n);
    bool sendAll (const void* buf, int n);
    bool recvLine(std::string& out, int maxLen = 256);
    bool sendLine(const std::string& l);

    socket_t handle() const { return s_; }

private:
    socket_t s_ = INVALID_SOCK;
};

// Host: listen, accept exactly one peer. Returns invalid socket on error.
TCPSocket net_host(int port, int timeout_sec = 60);

// Client: connect to host. Returns invalid socket on error.
TCPSocket net_join(const std::string& ip, int port);

} // namespace si::net
