#ifndef _WIN32

#include "platform.h"

#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <iostream>

namespace si::platform {

static struct termios g_old_term;

static int g_peeked = -1;

void net_init() {}
void net_cleanup() {}

void enable_ansi() {}
void hide_cursor() { std::cout << "\033[?25l"; }
void show_cursor() { std::cout << "\033[?25h"; }

void set_raw_mode(bool on) {
    if (on) {
        struct termios newt;
        tcgetattr(STDIN_FILENO, &g_old_term);
        newt = g_old_term;
        newt.c_lflag &= ~(ICANON | ECHO);
        newt.c_cc[VMIN]  = 0;
        newt.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_term);
    }
}

bool kb_available() {
    if (g_peeked != -1) return true;
    unsigned char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        g_peeked = static_cast<int>(ch);
        return true;
    }
    return false;
}

int read_key() {
    if (g_peeked != -1) {
        int c = g_peeked;
        g_peeked = -1;
        return c;
    }
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) <= 0) return -1;
    return static_cast<int>(ch);
}

void close_socket(socket_t s) { ::close(s); }
int  socket_errno() { return errno; }

}

#endif
