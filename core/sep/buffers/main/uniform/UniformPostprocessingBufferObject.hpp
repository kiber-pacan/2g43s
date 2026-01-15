#ifndef INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H
#define INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H

#include <glm/mat4x4.hpp>

struct UniformPostprocessingBufferObject {
    glm::vec2 resolution;
    float time = 0;
    float padding;
};

#endif //INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H