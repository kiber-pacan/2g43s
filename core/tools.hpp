#ifndef TOOLS_H
#define TOOLS_H

#include <random>
#include <chrono>
#include <iostream>

// Some tools
class tools {
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
};

#endif //TOOLS_H
