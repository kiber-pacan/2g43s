#ifndef INC_2G43S_UNIFORMBUFFEROBJECT_H
#define INC_2G43S_UNIFORMBUFFEROBJECT_H

#include <glm/mat4x4.hpp>

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

#endif //INC_2G43S_UNIFORMBUFFEROBJECT_H