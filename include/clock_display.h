#pragma once

#include <memory>
#include <string>
#include <vector>

class TimeZone;

class ClockDisplay {
private:
    static volatile bool running;
    int current_index;
    int width;
    std::vector<std::unique_ptr<TimeZone>> timezones;
    int system_offset;

    static void setCursorVisible(bool visible);
    static void signalHandler(int);
    int getSystemTimezoneOffset();
    void initTimezones();
    void calculateWidth();
    std::string getUTCTimeString();
    void printBorder(char style = '=');
    void printDivider();
    void printCentered(const std::string& text, bool with_border = true);
    void printProgressBar();
    void handleInput();
    void render();

public:
    ClockDisplay();
    ~ClockDisplay();
    void run();
};
