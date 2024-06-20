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
    int color;
    bool st = false;

    //I just want to create object what way because it is prettier for me
    static logger* of(const char* name, int color) {
        return new logger(name, color);
    }

    //Stub method for compiler
    static void log(const std::string& str) {
        std::cout << "" << std::endl;

    }

    //Main method for logging
    template<typename T>
    void log(const char* str, T value, auto... args) {
        start();
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

private:
    //Private constructor so ppl gonna use logger::of(name)
    explicit logger(const char* name, int color) {
        this->name = name;
        this->color = color;
    }

    //Method for printing (easier readability)
    void static print(auto x) {
        std::cout << x << std::endl;
    }

    void start() {
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
