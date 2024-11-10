#ifndef TOOLS_H
#define TOOLS_H

#include <random>
#include <chrono>
#include <fstream>
#include <iostream>

// Some tools
class Tools {
    public:

    // Handy dandy method for easy bounded random
    template<typename T>
    static T randomNum(auto min, auto max) {
        if (min > max) std::cerr << "you passed min that greater than max, you gonna get bad random results!" << std::endl;

        // Get time from epoch, we gonna use it as random seed
        const unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

        // Creating random engine with seed, so numbers never gonna repeat
        std::minstd_rand0 rand (seed);

        // Some old trick to bound random number
        const auto range = max - min + 1;
        T num = rand() % T(range + min);

        // returning the result
        return num;
    }

    // For reading shader files
    static std::vector<char> readFile(const char* filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
};

#endif //TOOLS_H
