#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <vector>

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
    bool st = false;

    enum Severity {
        INFO = 97,
        WARNING = 93,
        ERROR = 91
    };

    //I just want to create object what way because it is prettier for me
    static logger* of(const char* name) {
        return new logger(name);
    }

    //Main method for logging
    template<typename T>
    void printLog(int color, const char* str, T value, auto... args) {
        start(color);
        log(str, value, args...);
        st = false;
        std::cout << "" << std::endl ;
    }

private:
    //Stub method for compiler
    static void log(const std::string& str) {
        std::cout << "";
    }

    template<typename T>
    void log(const char* str, T value, auto... args) {
        /*
         *Basically erasing and printing char by char,
         *also replacing $ with value and then doing recursion with value as args[0]
        */
        for (; *str != '\0'; str++)
        {
            if (*str == '$')
            {
                std::cout << value;
                log(str + 1, args...);
                return;
            }
            std::cout << *str;
        }
    }

    //Private constructor so ppl gonna use logger::of(name)
    explicit logger(const char* name) {
        this->name = name;
    }

    //Start of print
    void start(int color) {
        if (!st) {
            st = true;
            std::cout
            << "\e[0;"
            << color
            << "m"
            << name
            <<": ";
        }
    }
};

#endif //LOGGER_H
