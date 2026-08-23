#include "clock_display.h"
#include "timezone.h"
#include "input.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

volatile bool ClockDisplay::running = true;

void ClockDisplay::setCursorVisible(bool visible) {
    std::cout << (visible ? "\033[?25h" : "\033[?25l");
    std::cout.flush();
}

void ClockDisplay::signalHandler(int) {
    running = false;
}

int ClockDisplay::getSystemTimezoneOffset() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    std::tm* utc = std::gmtime(&now);

    int offset = local->tm_hour - utc->tm_hour;
    if (offset < -12) offset += 24;
    if (offset > 12) offset -= 24;

    return offset;
}

void ClockDisplay::initTimezones() {
    for (int offset = -12; offset <= 12; offset++) {
        timezones.push_back(std::make_unique<TimeZone>(offset));
    }
    system_offset = getSystemTimezoneOffset();
    current_index = system_offset + 12;
}

void ClockDisplay::calculateWidth() {
    width = 55;

    if (current_index < timezones.size()) {
        std::string time_str = timezones[current_index]->getTimeString();
        std::string name_str = timezones[current_index]->getDisplayName();
        width = std::max(width, (int)name_str.length() + 8);
        width = std::max(width, (int)time_str.length() + 8);
    }

    std::string title = "WORLD CLOCK - Timezone Roller";
    std::string tip = "<- Prev   Next ->   r Refresh   q Quit";
    std::string utc_line = "UTC Time: " + getUTCTimeString();

    width = std::max({width, (int)title.length() + 6,
                      (int)tip.length() + 6, (int)utc_line.length() + 6});
}

std::string ClockDisplay::getUTCTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm* utc = std::gmtime(&now);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", utc);
    return std::string(buffer);
}

void ClockDisplay::printBorder(char style) {
    std::cout << "+" << std::string(width - 2, style) << "+" << std::endl;
}

void ClockDisplay::printDivider() {
    std::cout << "|" << std::string(width - 2, '-') << "|" << std::endl;
}

void ClockDisplay::printCentered(const std::string& text, bool with_border) {
    int padding = (width - 2 - text.length()) / 2;
    int padding_right = width - 2 - text.length() - padding;
    if (with_border) {
        std::cout << "|" << std::string(padding, ' ') << text
                  << std::string(padding_right, ' ') << "|" << std::endl;
    } else {
        std::cout << std::string(padding, ' ') << text
                  << std::string(padding_right, ' ') << std::endl;
    }
}

void ClockDisplay::printProgressBar() {
    int total = timezones.size();
    int pos = current_index;

    std::string bar = "  ";

    if (pos > 0) {
        bar += "<< ";
    } else {
        bar += "   ";
    }

    int start = std::max(0, pos - 5);
    int end = std::min(total - 1, pos + 5);

    for (int i = start; i <= end; i++) {
        if (i == pos) {
            bar += "[" + std::to_string(i - 12) + "]";
        } else {
            bar += " " + std::to_string(i - 12) + " ";
        }
    }

    if (pos < total - 1) {
        bar += " >>";
    } else {
        bar += "   ";
    }

    if (bar.length() > width - 4) {
        bar = bar.substr(0, width - 7) + "...";
    }

    int padding = width - 4 - bar.length();
    std::cout << "| " << bar << std::string(padding, ' ') << " |" << std::endl;
}

void ClockDisplay::handleInput() {
    int ch = read_key();
    if (ch < 0) return;

    if (ch & 0x100) {
        int arrow = ch & 0xFF;
        if (arrow == 0x4B || arrow == 0x44) {
            current_index = (current_index - 1 + timezones.size()) % timezones.size();
            std::cout << "\033[2J\033[H";
        } else if (arrow == 0x4D || arrow == 0x43) {
            current_index = (current_index + 1) % timezones.size();
            std::cout << "\033[2J\033[H";
        }
    } else {
        switch (ch) {
            case 'q': case 'Q': running = false; break;
            case 'r': case 'R': std::cout << "\033[2J\033[H"; break;
        }
    }
}

void ClockDisplay::render() {
    std::cout << "\033[H";

    printBorder('=');
    printCentered("WORLD CLOCK - Timezone Roller");
    printBorder('=');

    std::string display_line = timezones[current_index]->getDisplayName();
    printCentered(display_line);
    printCentered(timezones[current_index]->getTimeString());

    printDivider();
    printProgressBar();

    std::string utc_line = "UTC Time: " + getUTCTimeString();
    printCentered(utc_line);

    printBorder('=');
    printCentered("<- Prev   Next ->   r Refresh   q Quit");
    printBorder('=');

    std::cout.flush();
}

ClockDisplay::ClockDisplay() : current_index(0), width(55) {
    std::signal(SIGINT, signalHandler);

    std::cout << "\033[?1049h";
    std::cout.flush();

    set_raw_mode(true);
    setCursorVisible(false);
    initTimezones();

    std::cout << "\033[2J\033[H";
}

ClockDisplay::~ClockDisplay() {
    setCursorVisible(true);
    set_raw_mode(false);

    std::cout << "\033[?1049l";
    std::cout.flush();
}

void ClockDisplay::run() {
    while (running) {
        calculateWidth();
        render();
        handleInput();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
