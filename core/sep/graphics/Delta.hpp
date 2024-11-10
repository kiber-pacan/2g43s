#ifndef DELTA_H
#define DELTA_H
#include <SDL3/SDL_timer.h>

struct Delta {
    Delta() {
        now = 0, last = 0;
    }

    float now{}, last{};
    float d{};

    void calculateDelta() {
        last = now;
        now = (float) SDL_GetPerformanceCounter();
        d = (now - last) / (float) SDL_GetPerformanceFrequency();
    }
};

#endif //DELTA_H
