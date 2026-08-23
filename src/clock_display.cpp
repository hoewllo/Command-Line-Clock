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
    int max_width = std::max(12, columns - 2);

    if (detail == DetailLevel::Compact) {
        std::string line = timezones[current_index]->getCode() + "  " +
                           getShortTimeString(timezones[current_index]->getOffset());
        width = std::min(max_width, (int)line.length() + 4);
        width = std::max(width, 12);
        return;
    }

    if (detail == DetailLevel::Standard) {
        width = 55;
        std::string time_str = timezones[current_index]->getTimeString();
        std::string name_str = timezones[current_index]->getDisplayName();
        width = std::max(width, (int)name_str.length() + 8);
        width = std::max(width, (int)time_str.length() + 8);
        width = std::min(width, std::min(max_width, 72));
        return;
    }

    // Full: 撑满可用宽度，让时区表多列铺开
    width = std::max(60, max_width);
    width = std::min(width, 140);
}

std::string ClockDisplay::getUTCTimeString() {
    std::time_t now = std::time(nullptr);
    std::tm* utc = std::gmtime(&now);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", utc);
    return std::string(buffer);
}

std::string ClockDisplay::getShortTimeString(int offset) {
    std::time_t now = std::time(nullptr);
    std::time_t tz_time = now + offset * 3600;
    std::tm* tm = std::gmtime(&tz_time);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", tm);
    return std::string(buffer);
}

bool ClockDisplay::updateScreenSize() {
    int new_columns = get_terminal_columns();
    int new_rows = get_terminal_rows();
    bool changed = (new_columns != columns || new_rows != rows);
    columns = new_columns;
    rows = new_rows;

    DetailLevel new_detail;
    if (columns < 50 || rows < 12) {
        new_detail = DetailLevel::Compact;
    } else if (columns < 80 || rows < 20) {
        new_detail = DetailLevel::Standard;
    } else {
        new_detail = DetailLevel::Full;
    }

    if (new_detail != detail) {
        detail = new_detail;
        changed = true;
    }

    return changed;
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

void ClockDisplay::printAllTimezones() {
    std::vector<std::string> entries;
    int max_len = 0;
    for (size_t i = 0; i < timezones.size(); ++i) {
        std::string marker = (i == (size_t)current_index) ? "*" : " ";
        std::string entry = marker + timezones[i]->getDisplayName() + "  " +
                            timezones[i]->getTimeString();
        max_len = std::max(max_len, (int)entry.length());
        entries.push_back(entry);
    }

    int cols = std::max(1, (width - 2) / (max_len + 2));
    cols = std::min(cols, (int)entries.size());
    int col_w = (width - 2) / cols;

    size_t row_count = (entries.size() + cols - 1) / cols;
    for (size_t r = 0; r < row_count; ++r) {
        std::string line = "|";
        for (int c = 0; c < cols; ++c) {
            size_t idx = r * cols + c;
            std::string cell = (idx < entries.size()) ? entries[idx] : "";
            if ((int)cell.length() > col_w - 1) {
                cell = cell.substr(0, col_w - 1);
            }
            line += " " + cell + std::string(col_w - 1 - (int)cell.length(), ' ');
        }
        if ((int)line.length() < width - 1) {
            line += std::string(width - 1 - (int)line.length(), ' ');
        }
        line += "|";
        std::cout << line << std::endl;
    }
}

void ClockDisplay::renderCompact() {
    printBorder('=');
    printCentered(timezones[current_index]->getCode() + "  " +
                  getShortTimeString(timezones[current_index]->getOffset()));
    printBorder('=');
    printCentered("<- ->   q");
}

void ClockDisplay::renderStandard() {
    printBorder('=');
    printCentered("WORLD CLOCK - Timezone Roller");
    printBorder('=');

    printCentered(timezones[current_index]->getDisplayName());
    printCentered(timezones[current_index]->getTimeString());

    printDivider();
    printProgressBar();

    printBorder('=');
    printCentered("<- Prev   Next ->   r Refresh   q Quit");
    printBorder('=');
}

void ClockDisplay::renderFull() {
    printBorder('=');
    printCentered("WORLD CLOCK - Timezone Roller");
    printBorder('=');

    printCentered(timezones[current_index]->getDisplayName());
    printCentered(timezones[current_index]->getTimeString());

    printDivider();
    printProgressBar();

    std::string utc_line = "UTC Time: " + getUTCTimeString();
    printCentered(utc_line);

    printDivider();
    printAllTimezones();

    printBorder('=');
    printCentered("<- Prev   Next ->   r Refresh   q Quit");
    printBorder('=');
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

    switch (detail) {
        case DetailLevel::Compact:
            renderCompact();
            break;
        case DetailLevel::Standard:
            renderStandard();
            break;
        case DetailLevel::Full:
            renderFull();
            break;
    }

    std::cout.flush();
}

ClockDisplay::ClockDisplay() : current_index(0), width(55), columns(0), rows(0),
                               detail(DetailLevel::Standard) {
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
        if (updateScreenSize()) {
            std::cout << "\033[2J\033[H";
        }
        calculateWidth();
        render();
        handleInput();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
