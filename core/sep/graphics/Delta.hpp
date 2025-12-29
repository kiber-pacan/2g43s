#ifndef DELTA_H
#define DELTA_H
#include <SDL3/SDL_timer.h>
#include <cstddef>
#include <numeric>
#include <vector>
#include <iostream>

struct Delta {
    Delta() {
        now = 0, last = 0;
    }

    float now{}, last{};
    float d{};

    std::vector<double> fpsHistory{};
    float maxHistory = 10.0f;
    float fps = 0.0f;
    
    void calculateDelta() {
        last = now;
        now = SDL_GetTicks();
        d = (float) (now - last) / 1000.0;
        
        if (fpsHistory.size() >= maxHistory) {
            fps = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0) / maxHistory;
            fpsHistory.clear();
        } else {
            fpsHistory.emplace_back(1 / d);
        }        
    }
};

#endif //DELTA_H
