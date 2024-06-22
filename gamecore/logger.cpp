#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

/*
foreground background
black   30 40
red     31 41
green   32 42
yellow  33 43
blue    34 44
magenta 35 45
cyan    36 46
white   37 47
*/

//Just look at names of class and file...
class logger {
public:
    const char* name;
    //For ASCI start of print
    bool st = false;

    //Color for severity of log
    struct severity {
        static constexpr auto LOG = "ffffff";
        static constexpr auto WARNING = "eed202";
        static constexpr auto ERROR = "ff1100";
        static constexpr auto SUCCESS = "76ff00";
    };

    //I just want to create object what way because it is prettier for me
    static logger* of(const char* name) {
        return new logger(name);
    }

    //Main method for logging
    template<typename T>
    void log(const char* hex, const char* str, T value, auto... args) {
        start(hex);
        tempLog(str, value, args...);
        st = false;
        std::cout << "" << std::endl ;
    }

private:
    //Stub method for recursion
    static void tempLog(const std::string& str) {
        std::cout << "";
    }

    //Method just for printing messages
    template<typename T>
    void tempLog(const char* str, T value, auto... args) {
        /*
         *Basically erasing and printing char by char,
         *also replacing $ with value and then doing recursion with value as args[0]
        */
        for (; *str != '\0'; str++)
        {
            if (*str == '$')
            {
                std::cout << value;
                tempLog(str + 1, args...);
                return;
            }
            std::cout << *str;
        }
    }

    //Private constructor so ppl gonna use logger::of(name)
    explicit logger(const char* name) {
        this->name = name;
    }

    //Start of log (Color and name)
    void start(const char* hex) {
        if (!st) {

            const std::string r{hex[0], hex[1]};
            const std::string g{hex[2], hex[3]};
            const std::string b{hex[4], hex[5]};
            st = true;
            std::cout
            << "\033[38;2;"
            << std::stoi(r, nullptr, 16) << ";"
            << std::stoi(g, nullptr, 16) << ";"
            << std::stoi(b, nullptr, 16) << "m"
            << name << ": ";
        }
    }
    void end() {

    }
};

#endif //LOGGER_H
