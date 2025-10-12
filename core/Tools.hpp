#ifndef TOOLS_H
#define TOOLS_H

#include <random>
#include <chrono>
#include <fstream>
#include <iostream>

// Some tools
class Tools {
    public:
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
