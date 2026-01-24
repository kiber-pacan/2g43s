#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>

#include "Color.hpp"
#include <sstream>

// Just look at names of class and file...
class Logger {
public:
    const char* name{};

    // Color for severity of log
    struct severity {
        static constexpr Color LOG = Color::CONSTEXPR_HEX(0xffffff);
        static constexpr Color WARNING = Color::CONSTEXPR_HEX(0xeed202);
        static constexpr Color ERROR = Color::CONSTEXPR_HEX(0xff1100);
        static constexpr Color SUCCESS = Color::CONSTEXPR_HEX(0x76ff00);
    };

    Logger() = default;

    explicit Logger(const char* name) {
        this->name = name;
    }



    #pragma region info
    // Main method for logging
    template<typename T, typename T1>
    void info(T1 str, T value, auto... args) {
        if constexpr (std::is_arithmetic_v<T1>)
            log(severity::LOG, std::to_string(str), value, args...);
        else
            log(severity::LOG, str, value, args...);
    }

    template<typename T1>
    void info(T1 str) {
        log(severity::LOG, str);
    }
    #pragma endregion


    #pragma region warn
    template<typename T, typename T1>
    void warn(T1 str, T value, auto... args) {
        if constexpr (std::is_arithmetic_v<T1>)
            log(severity::WARNING, std::to_string(str), value, args...);
        else
            log(severity::WARNING, str, value, args...);
    }

    template<typename T1>
    void warn(T1 str) {
        log(severity::WARNING, str);
    }
    #pragma endregion


    #pragma region error
    template<typename T, typename T1>
    void error(T1 str, T value, auto... args) {
        if constexpr (std::is_arithmetic_v<T1>)
            log(severity::ERROR,std::to_string(str), value, args...);
        else
            log(severity::ERROR,str, value, args...);
    }

    template<typename T1>
    void error(T1 str) {
        log(severity::ERROR, str);
    }
    #pragma endregion


    #pragma region success
    template<typename T, typename T1>
    void success(T1 str, T value, auto... args) {
        if constexpr (std::is_arithmetic_v<T1>)
            log(severity::SUCCESS, std::to_string(str), value, args...);
        else
            log(severity::SUCCESS, str, value, args...);
    }

    template<typename T1>
    void success(T1 str) {
        log(severity::SUCCESS, str);
    }
    #pragma endregion

private:
    // Stub methods
    template<typename T, typename T1>
    void log(const Color& color, T1 str, T value, auto... args) {
        std::stringstream ss{};

        start(ss, color); // Set formatting

        buildString(ss, str, value, args...); // Replace ${}

        end(ss); // Reset coloring and start from new line

        // Print string
        std::cout << ss.str();
        std::cout.flush();
    }

    template<typename T>
    void log(const Color& color, T str) {
        std::stringstream ss{};

        start(ss, color);  // Set formatting
        ss << str;
        end(ss); // Reset coloring and start from new line

        // Print string
        std::cout << ss.str();
        std::cout.flush();
    }

    // Stub fella
    void buildString(std::stringstream& ss, const char* str) {}


    // Method just for printing messages
    template<typename T>
    void buildString(std::stringstream& ss, const char* str, T value, auto... args) {
        // Basically adding to ss char by char,
        for (; *str != '\0'; str++) {
            // Also replacing ${} with value and then doing recursion with value as args[0]
            if (str[0] == '$' && str[1] == '{' && str[2] == '}') {
                str += 3; // Skipping length of ${} aka 3, so the next char is after ${} construction

                ss << value;

                // Preventing from recursing with empty args and the end of str
                if (sizeof...(args) > 0 || *str == '\0') {
                    buildString(ss, str, args...);
                    return;
                }
            }

            ss << *str;
        }
    }

    // Start of ss (Color and name)
    void start(std::stringstream& ss, const Color& color) const {
        ss << "\033[38;2;"
        << static_cast<int>(color.r) << ";"
        << static_cast<int>(color.g) << ";"
        << static_cast<int>(color.b) << "m"
        << name << ": ";
    }

    // Resets coloring and starts from new line
    void end(std::stringstream& ss) {
        ss << "\033[0m\n";
    }
};

#endif //LOGGER_H
