#include "timezone.h"

#include <ctime>
#include <cstdio>
#include <map>
#include <string>

TimeZone::TimeZone(int offset) : offset_hours(offset) {
    std::map<int, std::string> tz_codes = {
        {-12, "BIT"}, {-11, "SST"}, {-10, "HST"}, {-9, "AKST"},
        {-8, "PST"}, {-7, "MST"}, {-6, "CST"}, {-5, "EST"},
        {-4, "AST"}, {-3, "BRT"}, {-2, "FNT"}, {-1, "CVT"},
        {0, "UTC"}, {1, "CET"}, {2, "CAT"}, {3, "MSK"},
        {4, "GST"}, {5, "PKT"}, {6, "BST"}, {7, "WIB"},
        {8, "CST"}, {9, "JST"}, {10, "AEST"}, {11, "SBT"}, {12, "NZST"}
    };

    if (tz_codes.find(offset) != tz_codes.end()) {
        code = tz_codes[offset];
    } else {
        std::string sign = (offset >= 0) ? "+" : "";
        code = "UTC" + sign + std::to_string(offset);
    }

    if (offset == 0) {
        name = "UTC";
    } else if (offset > 0) {
        name = "UTC+" + std::to_string(offset) + " " + code;
    } else {
        name = "UTC" + std::to_string(offset) + " " + code;
    }
}

std::string TimeZone::getName() const { return name; }
std::string TimeZone::getCode() const { return code; }
int TimeZone::getOffset() const { return offset_hours; }

std::string TimeZone::getTimeString() {
    std::time_t now = std::time(nullptr);
    std::time_t tz_time = now + offset_hours * 3600;
    std::tm* tm = std::gmtime(&tz_time);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", tm);
    return std::string(buffer);
}

std::string TimeZone::getDisplayName() const {
    if (offset_hours == 0) {
        return "UTC";
    } else if (offset_hours > 0) {
        return "UTC+" + std::to_string(offset_hours) + " " + code;
    } else {
        return "UTC" + std::to_string(offset_hours) + " " + code;
    }
}
