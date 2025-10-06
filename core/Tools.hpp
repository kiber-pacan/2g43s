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
    static T randomNum(T min, T max) {
        static std::random_device rd; // максимально случайное семя от системы
        static std::mt19937_64 rng(rd()); // генератор с большим периодом

        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(rng);
        } else {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(rng);
        }
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
