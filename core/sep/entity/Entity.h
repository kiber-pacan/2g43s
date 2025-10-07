#ifndef ENTITY_H
#define ENTITY_H
#include <utility>

#include "glm/vec3.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


struct Entity {
    virtual ~Entity() = default;

    glm::mat4 mdlMat{1.0f};


    explicit Entity(
        const glm::vec3 p = glm::vec3(0.0f),
        const glm::vec3 r = glm::vec3(0.0f),
        const glm::vec3 s = glm::vec3(1.0f)
        ) {
        // Pos
        mdlMat = glm::translate(mdlMat, p);

    }

    virtual void respawn() {
        mdlMat = glm::mat4(1.0f);
    }
};

#endif //ENTITY_H
