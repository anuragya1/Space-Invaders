#pragma once

#include <cstdint>
#include <chrono>
#include <thread>

#ifdef _WIN32

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    using socket_t = SOCKET;
    inline constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
    #define SI_CLEAR_CMD "cls"
#else
    using socket_t = int;
    inline constexpr socket_t INVALID_SOCK = -1;
    #define SI_CLEAR_CMD "clear"
#endif

namespace si::platform {

void net_init();
void net_cleanup();

void enable_ansi();
void hide_cursor();
void show_cursor();
void set_raw_mode(bool on);

bool kb_available();
int  read_key();

void close_socket(socket_t s);
int  socket_errno();

inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}
