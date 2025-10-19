#ifndef INC_2G43S_VISIBLEINDICESBUFFEROBJECT_H
#define INC_2G43S_VISIBLEINDICESBUFFEROBJECT_H

#include <atomic>
#include <vector>
#include <glm/mat4x4.hpp>

struct VisibleIndicesBufferObject {
    std::vector<uint32_t> vi;
};

#endif //INC_2G43S_VISIBLEINDICESBUFFEROBJECT_H