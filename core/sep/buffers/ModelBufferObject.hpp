#ifndef INC_2G43S_MODELBUFFEROBJECT_H
#define INC_2G43S_MODELBUFFEROBJECT_H

#include <vector>
#include <glm/mat4x4.hpp>

struct ModelBufferObject {
    std::vector<glm::mat4> mdls;
};

#endif //INC_2G43S_MODELBUFFEROBJECT_H