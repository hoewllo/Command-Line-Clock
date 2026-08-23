#pragma once

#include <string>

class TimeZone {
private:
    int offset_hours;
    std::string name;
    std::string code;

public:
    TimeZone(int offset);

    std::string getName() const;
    std::string getCode() const;
    int getOffset() const;

    std::string getTimeString();
    std::string getDisplayName() const;
};
