#ifndef INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H
#define INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H

#include <glm/mat4x4.hpp>

struct UniformPostprocessingBufferObject {
    glm::vec2 resolution;
    double time = 0;
};

#endif //INC_2G43S_UNIFORMPOSTPROCESSINGBUFFEROBJECT_H