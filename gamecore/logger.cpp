#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <vector>

//Just look at names of class and file...
class logger {
public:
    const char* name;
    std::vector<const char*> chars;

    //I just want to create object what way because it is prettier for me
    static logger* of(const char* name) {
        return new logger(name);
    }

    void log(const std::string& str) {
        std::cout << "" << std::endl;
    }


    //Main method for logging
    template<typename T, typename... T1>
    void log(const char* str, T value, T1... args) {
        std::cout << "\033[1;31m";

        for (; *str != '\0'; str++)
        {
            if (*str == '$')
            {
                std::cout << value;
                log(str + 1, args...); // recursive call
                return;
            }
            std::cout << *str;
        }
        std::cout << "\033[0m\n";
    }
private:
    //Private constructor so ppl gonna use logger::of(name)
    explicit logger(const char* name) {
        this->name = name;
    }

    //Method for printing (easier readability)
    void static print(auto x) {
        std::cout << x << std::endl;
    }
};

#endif //LOGGER_H
