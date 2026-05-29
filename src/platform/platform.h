// platform.h - OS abstraction interface
// Provides a uniform API for terminal control, non-blocking input,
// and BSD/Winsock-style sockets across Windows and POSIX.
//
// All platform-specific includes live in the .cpp files; only the
// public sockets_t type and constants leak through this header.

#pragma once

#include <cstdint>
#include <chrono>
#include <thread>

#ifdef _WIN32
    // Tame the windows.h header before it pollutes the global namespace.
    // WIN32_LEAN_AND_MEAN drops things we don't need (cryptography, RPC, etc).
    // NOMINMAX prevents the `min`/`max` macros that conflict with std::min/max.
    // We also #undef ERROR / DEBUG defensively in logger.h.
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

// One-time process startup / teardown (Winsock init, etc.).
void net_init();
void net_cleanup();

// Terminal control.
void enable_ansi();          // No-op on POSIX, enables VT processing on Win10+.
void hide_cursor();
void show_cursor();
void set_raw_mode(bool on);  // POSIX termios; no-op on Windows (conio is already raw).

// Non-blocking single-byte keyboard polling.
//
// kb_available() returns true if a byte is ready *and* peeks it into a
// shared buffer; the next read_key() returns that same byte. This pairing
// is necessary because POSIX read(STDIN_FILENO,...) and stdio do not share
// buffers - ungetc() would be invisible to the next read().
bool kb_available();
int  read_key();              // returns -1 on EOF / no data

// Socket close (platform-specific name).
void close_socket(socket_t s);
int  socket_errno();

// Sleep helper (millis).
inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace si::platform
