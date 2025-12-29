#ifndef INC_2G43S_RANDOM_H
#define INC_2G43S_RANDOM_H
#include <random>
#include <omp.h>

struct Random {
    inline static std::random_device rD{};

    inline static std::mt19937_64 rE{rD()};
    inline static std::vector<std::default_random_engine> rD_T = [] {
        // Creating random engine for each thread in advance
        std::random_device r;
        const size_t threads = omp_get_max_threads();

        std::vector<std::default_random_engine> temp;
        temp.resize(threads);
        for (int i = 0; i < threads; ++i) {
            temp[i] = std::default_random_engine(r());
        }
        return temp;
    }();

    // Basic random function
    template<typename T>
    static T randomNum(T min, T max) {
        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(rE);
        } else {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(rE);
        }
    }

    // Thread safe randomNum function
    template<typename T>
    static T randomNum_T(T min, T max) {
        thread_local std::mt19937_64 rE_T{std::random_device{}()};

        if constexpr (std::is_integral_v<T>) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(rE_T);
        } else {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(rE_T);
        }
    }
};



#endif //INC_2G43S_RANDOM_H