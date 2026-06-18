//
// Created by down1 on 11.04.2026.
//

#ifndef INC_2G43S_MODELREGION_H
#define INC_2G43S_MODELREGION_H
#include "glm/fwd.hpp"

#endif //INC_2G43S_MODELREGION_H

struct ModelRegion {
    explicit ModelRegion(uint32_t modelIndex) : modelIndex(modelIndex) {}

    uint32_t offset = 0; // First index
    uint32_t count = 0; // Instances count
    uint32_t modelIndex{}; // Index of model in vector
    uint32_t pad{};
};