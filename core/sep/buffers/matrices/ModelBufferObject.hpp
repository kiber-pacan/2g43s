#ifndef INC_2G43S_MODELBUFFEROBJECT_HPP
#define INC_2G43S_MODELBUFFEROBJECT_HPP

#include <vector>
#include <glm/mat4x4.hpp>

struct ModelBufferObject {
    std::vector<glm::mat4> models;
};

#endif //INC_2G43S_MODELBUFFEROBJECT_HPP