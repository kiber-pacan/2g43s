#ifndef DELTA_H
#define DELTA_H

#include <vector>

struct DeltaManager {
    DeltaManager() {
        now = 0, last = 0;
    }
    double deltaTime{};
    double frameRate = 0.0f;

    void calculateDelta();
private:
    double now{}, last{};

    std::vector<double> fpsHistory{};
    double maxHistory = 30.0;
};

#endif //DELTA_H
