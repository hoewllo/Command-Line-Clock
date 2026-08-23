#include "input.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

#ifdef _WIN32

void set_raw_mode(bool raw) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    static DWORD orig_mode;
    if (raw) {
        GetConsoleMode(hStdin, &orig_mode);
        DWORD mode = orig_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        SetConsoleMode(hStdin, mode);
    } else {
        SetConsoleMode(hStdin, orig_mode);
    }
}

int read_key() {
    if (!_kbhit()) return -1;
    int ch = _getch();
    if (ch == 0xE0) {
        ch = _getch();
        return ch | 0x100;
    }
    return ch;
}

#else

struct termios orig_termios;

void set_raw_mode(bool raw) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    if (raw) {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        ttystate.c_lflag &= ~(ICANON | ECHO);
        ttystate.c_cc[VMIN] = 0;
        ttystate.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
}

int read_key() {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) != 1) return -1;
    if (ch == 27) {
        char seq[2];
        if (read(STDIN_FILENO, &seq, 2) == 2 && seq[0] == '[') {
            return seq[1] | 0x100;
        }
        return 27;
    }
    return static_cast<unsigned char>(ch);
}

#endif

int get_terminal_columns() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return 80;
#endif
}

int get_terminal_rows() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    return 24;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        return ws.ws_row;
    }
    return 24;
#endif
}
