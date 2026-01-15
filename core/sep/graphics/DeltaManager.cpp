#include "DeltaManager.hpp"

#include <numeric>
#include <SDL3/SDL_timer.h>

void DeltaManager::calculateDelta() {
    last = now;
    now = static_cast<double>(SDL_GetTicksNS());
    deltaTime = (now - last) / 1000000000.0;

    if (fpsHistory.size() >= static_cast<int>(maxHistory)) {
        frameRate = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0.0) / maxHistory;
        fpsHistory.clear();
    } else {
        fpsHistory.emplace_back(1 / deltaTime);
    }
}