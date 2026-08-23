#pragma once

#include <memory>
#include <string>
#include <vector>

class TimeZone;

enum class DetailLevel {
    Compact,
    Standard,
    Full
};

class ClockDisplay {
private:
    static volatile bool running;
    int current_index;
    int width;
    int columns;
    int rows;
    DetailLevel detail;
    std::vector<std::unique_ptr<TimeZone>> timezones;
    int system_offset;

    static void setCursorVisible(bool visible);
    static void signalHandler(int);
    bool updateScreenSize();
    int getSystemTimezoneOffset();
    void initTimezones();
    void calculateWidth();
    std::string getUTCTimeString();
    std::string getShortTimeString(int offset);
    void printBorder(char style = '=');
    void printDivider();
    void printCentered(const std::string& text, bool with_border = true);
    void printProgressBar();
    void printAllTimezones();
    void renderCompact();
    void renderStandard();
    void renderFull();
    void handleInput();
    void render();

public:
    ClockDisplay();
    ~ClockDisplay();
    void run();
};
