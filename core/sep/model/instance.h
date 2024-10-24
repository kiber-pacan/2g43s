#ifndef INSTANCE_H
#define INSTANCE_H
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"
#include "glm/detail/type_quat.hpp"
#include "model.h"

struct mdlInstance {
    mdl model{};
    glm::vec3 pos{};
    glm::quat quat{};
};

#endif //INSTANCE_H
