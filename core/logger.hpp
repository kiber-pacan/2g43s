#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>

// Just look at names of class and file...
class logger {
public:
    const char* name;

    // Color for severity of log
    struct severity {
        static constexpr auto LOG = "ffffff";
        static constexpr auto WARNING = "eed202";
        static constexpr auto ERROR = "ff1100";
        static constexpr auto SUCCESS = "76ff00";
    };

    // I just want to create object what way because it is prettier for me
    static logger* of(const char* name) {
        return new logger(name);
    }

    // Main method for logging
    template<typename T>
    void log(const char* hex, const char* str, T value, auto... args) {
        // Coloring
        start(hex);

        // Printing
        tempLog(str, value, args...);

        // Reset coloring
        std::cout << "\033[0m" << std::endl;
    }

private:
    // Stub method for recursion
    static void tempLog(const std::string& str) {
        std::cout << "";
    }

    // Method just for printing messages
    template<typename T>
    void tempLog(const char* str, T value, auto... args) {
        /*
         *Basically erasing and printing char by char,
         *also replacing $ with value and then doing recursion with value as args[0]
        */
        for (; *str != '\0'; str++) {
            if (*str == '$') {
                std::cout << value;
                tempLog(str + 1, args...);
                return;
            }
            std::cout << *str;
        }
    }

    // Private constructor so ppl gonna use logger::of(name)
    explicit logger(const char* name) {
        this->name = name;
    }

    // Start of log (Color and name)
    void start(const char* hex) {
        // Raw hex colors
        const std::string r{hex[0], hex[1]};
        const std::string g{hex[2], hex[3]};
        const std::string b{hex[4], hex[5]};
        std::cout
        << "\033[38;2;"
        << std::stoi(r, nullptr, 16) << ";"
        << std::stoi(g, nullptr, 16) << ";"
        << std::stoi(b, nullptr, 16) << "m"
        << name << ": ";
    }
};

#endif //LOGGER_H
