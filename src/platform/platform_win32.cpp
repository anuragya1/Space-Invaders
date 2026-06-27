#ifdef _WIN32

#include "platform.h"

#include <windows.h>
#include <conio.h>
#include <ws2tcpip.h>
#include <iostream>

#ifdef _MSC_VER
#  pragma comment(lib, "ws2_32.lib")
#endif

namespace si::platform {

void net_init() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}
void net_cleanup() { WSACleanup(); }

void enable_ansi() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void hide_cursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci{1, FALSE};
    SetConsoleCursorInfo(h, &ci);
}

void show_cursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci{1, TRUE};
    SetConsoleCursorInfo(h, &ci);
}

void set_raw_mode(bool) {}

bool kb_available() { return _kbhit() != 0; }
int  read_key()     { return _getch(); }

void close_socket(socket_t s) { closesocket(s); }
int  socket_errno() { return WSAGetLastError(); }

}

#endif
