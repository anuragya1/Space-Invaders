// tcp_socket.cpp
#include "tcp_socket.h"
#include "../debug/logger.h"
#include "../core/colors.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <sys/select.h>  // fixes macOS build error because select() needs this header
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

namespace si::net {

TCPSocket::~TCPSocket() {
    if (s_ != INVALID_SOCK) platform::close_socket(s_);
}

TCPSocket::TCPSocket(TCPSocket&& o) noexcept : s_(o.s_) { o.s_ = INVALID_SOCK; }

TCPSocket& TCPSocket::operator=(TCPSocket&& o) noexcept {
    if (this != &o) {
        if (s_ != INVALID_SOCK) platform::close_socket(s_);
        s_   = o.s_;
        o.s_ = INVALID_SOCK;
    }
    return *this;
}

bool TCPSocket::recvAll(void* buf, int n) {
    char* p = (char*)buf;
    int left = n;
    while (left > 0) {
        int r = ::recv(s_, p, left, 0);
        if (r <= 0) return false;
        p    += r;
        left -= r;
    }
    return true;
}

bool TCPSocket::sendAll(const void* buf, int n) {
    const char* p = (const char*)buf;
    int left = n;
    while (left > 0) {
        int r = ::send(s_, p, left, 0);
        if (r <= 0) return false;
        p    += r;
        left -= r;
    }
    return true;
}

bool TCPSocket::recvLine(std::string& out, int maxLen) {
    out.clear();
    char ch;
    for (int i = 0; i < maxLen; ++i) {
        int r = ::recv(s_, &ch, 1, 0);
        if (r <= 0)    return false;
        if (ch == '\n') return true;
        out += ch;
    }
    return false;
}

bool TCPSocket::sendLine(const std::string& l) {
    std::string m = l + '\n';
    return sendAll(m.data(), (int)m.size());
}

// Host/join helpers.

TCPSocket net_host(int port, int timeout_sec) {
    TCPSocket none;
    socket_t srv = ::socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCK) {
        LOG_ERROR("socket() failed: " << platform::socket_errno());
        return none;
    }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    sockaddr_in a{};
    a.sin_family      = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port        = htons((uint16_t)port);
    if (::bind(srv, (sockaddr*)&a, sizeof(a)) < 0) {
        LOG_ERROR("bind() failed: " << platform::socket_errno());
        platform::close_socket(srv);
        return none;
    }
    if (::listen(srv, 1) < 0) {
        LOG_ERROR("listen() failed: " << platform::socket_errno());
        platform::close_socket(srv);
        return none;
    }

    LOG_INFO("listening on port " << port);
    std::cout << color::BCYAN
              << "  Listening on port " << port << " ... waiting for peer.\n"
              << "  (give your friend your IP. Ctrl-C to cancel.)\n"
              << color::RST;

    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(srv, &rd);
    timeval tv{ timeout_sec, 0 };
    int sel = ::select((int)srv + 1, &rd, nullptr, nullptr, &tv);
    if (sel <= 0) {
        LOG_WARN("accept timed out");
        platform::close_socket(srv);
        return none;
    }

    sockaddr_in cli{};
#ifdef _WIN32
    int clen = sizeof(cli);
#else
    socklen_t clen = sizeof(cli);
#endif
    socket_t c = ::accept(srv, (sockaddr*)&cli, &clen);
    platform::close_socket(srv);
    if (c == INVALID_SOCK) {
        LOG_ERROR("accept() failed");
        return none;
    }
    LOG_INFO("peer connected");
    return TCPSocket(c);
}

TCPSocket net_join(const std::string& ip, int port) {
    TCPSocket none;
    socket_t c = ::socket(AF_INET, SOCK_STREAM, 0);
    if (c == INVALID_SOCK) {
        LOG_ERROR("socket() failed");
        return none;
    }
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port   = htons((uint16_t)port);
#ifdef _WIN32
    a.sin_addr.s_addr = inet_addr(ip.c_str());
    if (a.sin_addr.s_addr == INADDR_NONE) { platform::close_socket(c); return none; }
#else
    if (inet_pton(AF_INET, ip.c_str(), &a.sin_addr) <= 0) {
        LOG_ERROR("inet_pton('" << ip << "') failed");
        platform::close_socket(c);
        return none;
    }
#endif
    if (::connect(c, (sockaddr*)&a, sizeof(a)) < 0) {
        int err = platform::socket_errno();
        const char* msg =
#ifdef _WIN32
            (err == WSAECONNREFUSED) ? "connection refused (is the host running?)" :
            (err == WSAETIMEDOUT)    ? "connection timed out (wrong IP / blocked by firewall?)" :
                                       "connect failed";
#else
            std::strerror(err);
#endif
        LOG_ERROR("connect() failed: " << err << " (" << msg << ")");
        std::cerr << "  connect error: " << msg << " [errno=" << err << "]\n";
        platform::close_socket(c);
        return none;
    }
    LOG_INFO("connected to " << ip << ":" << port);
    return TCPSocket(c);
}

} // namespace si::net
