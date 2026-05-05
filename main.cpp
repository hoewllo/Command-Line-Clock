#include <iostream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <csignal>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <map>

// 时区类
class TimeZone {
private:
    int offset_hours;
    std::string name;
    std::string code;
    
public:
    TimeZone(int offset) : offset_hours(offset) {
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
    
    std::string getName() const { return name; }
    std::string getCode() const { return code; }
    int getOffset() const { return offset_hours; }
    
    std::string getTimeString() {
        std::time_t now = std::time(nullptr);
        std::time_t tz_time = now + offset_hours * 3600;
        std::tm* tm = std::gmtime(&tz_time);
        
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", tm);
        return std::string(buffer);
    }
    
    std::string getDisplayName() const {
        if (offset_hours == 0) {
            return "UTC";
        } else if (offset_hours > 0) {
            return "UTC+" + std::to_string(offset_hours) + " " + code;
        } else {
            return "UTC" + std::to_string(offset_hours) + " " + code;
        }
    }
};

// 时钟显示器类
class ClockDisplay {
private:
    static volatile bool running;
    int current_index;
    int width;
    std::vector<std::unique_ptr<TimeZone>> timezones;
    int system_offset;
    struct termios original_termios;
    
    static void setTerminalMode(bool raw) {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);
        if (raw) {
            ttystate.c_lflag &= ~(ICANON | ECHO);
            ttystate.c_cc[VMIN] = 0;
            ttystate.c_cc[VTIME] = 0;
        } else {
            ttystate.c_lflag |= ICANON | ECHO;
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    }
    
    static void setCursorVisible(bool visible) {
        std::cout << (visible ? "\033[?25h" : "\033[?25l");
        std::cout.flush();
    }
    
    static void signalHandler(int) {
        running = false;
    }
    
    int getSystemTimezoneOffset() {
        std::time_t now = std::time(nullptr);
        std::tm* local = std::localtime(&now);
        std::tm* utc = std::gmtime(&now);
        
        int offset = local->tm_hour - utc->tm_hour;
        if (offset < -12) offset += 24;
        if (offset > 12) offset -= 24;
        
        return offset;
    }
    
    void initTimezones() {
        for (int offset = -12; offset <= 12; offset++) {
            timezones.push_back(std::make_unique<TimeZone>(offset));
        }
        system_offset = getSystemTimezoneOffset();
        current_index = system_offset + 12;
    }
    
    void calculateWidth() {
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
    
    std::string getUTCTimeString() {
        std::time_t now = std::time(nullptr);
        std::tm* utc = std::gmtime(&now);
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", utc);
        return std::string(buffer);
    }
    
    void printBorder(char style = '=') {
        std::cout << "+" << std::string(width - 2, style) << "+" << std::endl;
    }
    
    void printDivider() {
        std::cout << "|" << std::string(width - 2, '-') << "|" << std::endl;
    }
    
    void printCentered(const std::string& text, bool with_border = true) {
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
    
    void printProgressBar() {
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
    
    void handleInput() {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            switch(ch) {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case 'r':
                case 'R':
                    std::cout << "\033[2J\033[H";
                    break;
                case 27: {
                    char seq[2];
                    if (read(STDIN_FILENO, &seq, 2) == 2) {
                        if (seq[0] == '[') {
                            if (seq[1] == 'D') {
                                current_index = (current_index - 1 + timezones.size()) % timezones.size();
                                std::cout << "\033[2J\033[H";
                            } else if (seq[1] == 'C') {
                                current_index = (current_index + 1) % timezones.size();
                                std::cout << "\033[2J\033[H";
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    
    void render() {
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
    
public:
    ClockDisplay() : current_index(0), width(55) {
        tcgetattr(STDIN_FILENO, &original_termios);
        
        std::signal(SIGINT, signalHandler);
        
        // 保存当前屏幕内容（使用 alternate screen buffer）
        std::cout << "\033[?1049h";  // 切换到备用屏幕
        std::cout.flush();
        
        setTerminalMode(true);
        setCursorVisible(false);
        initTimezones();
        
        std::cout << "\033[2J\033[H";  // 清空备用屏幕
    }
    
    ~ClockDisplay() {
        // 恢复终端设置
        setCursorVisible(true);
        setTerminalMode(false);
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        
        // 切换回主屏幕，备用屏幕自动消失
        std::cout << "\033[?1049l";
        std::cout.flush();
    }
    
    void run() {
        while (running) {
            calculateWidth();
            render();
            handleInput();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

volatile bool ClockDisplay::running = true;

int main() {
    ClockDisplay display;
    display.run();
    return 0;
}
