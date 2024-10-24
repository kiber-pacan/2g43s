#ifndef ENTITY_H
#define ENTITY_H
#include "glm/vec3.hpp"

struct entity {
    glm::vec3 pos{};
    glm::vec3 look{};

    virtual void respawn() {
        pos = glm::vec3();
        look = glm::vec3(1,0,0);
    }
};

#endif //ENTITY_H
