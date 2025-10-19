#ifndef INC_2G43S_MODELDATABUFFEROBJECT_H
#define INC_2G43S_MODELDATABUFFEROBJECT_H
#define INC_2G43S_MODELBUFFEROBJECT_H

#include <vector>
#include <glm/mat4x4.hpp>

struct ModelDataBufferObject {
    std::vector<glm::vec4> pos;
    std::vector<glm::vec4> rot;
    std::vector<glm::vec4> scl;
    std::vector<glm::vec4> sphere;
    bool dirty2 = true;
    size_t frame = 0;
};

#endif //INC_2G43S_MODELDATABUFFEROBJECT_H