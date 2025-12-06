#ifndef INC_2G43S_UNIFORMCULLINGBUFFEROBJECT_H
#define INC_2G43S_UNIFORMCULLINGBUFFEROBJECT_H

#include <glm/mat4x4.hpp>

struct UniformCullingBufferObject {
    glm::vec4 planes[6];
    uint32_t totalObjects;
};

#endif //INC_2G43S_UNIFORMCULLINGBUFFEROBJECT_H