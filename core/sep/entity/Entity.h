#ifndef ENTITY_H
#define ENTITY_H
#include <utility>

#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Entity {
    virtual ~Entity() = default;

    std::string name;

    glm::mat4 mdlMat{1.0f};


    explicit Entity(
        const glm::vec3 p = glm::vec3(0.0f),
        const glm::vec3 r = glm::vec3(0.0f),
        const glm::vec3 s = glm::vec3(1.0f),
        std::string n = "entity-" + std::to_string(Tools::randomNum<uint32_t>(10000, 99999))
        ) : name(std::move(n)) {
        // Pos
        mdlMat = glm::translate(mdlMat, p);

        // Rot
        mdlMat = glm::rotate(mdlMat, r.x, glm::vec3(1.0f, 0.0f, 0.0f));
        mdlMat = glm::rotate(mdlMat, r.y, glm::vec3(0.0f, 1.0f, 0.0f));
        mdlMat = glm::rotate(mdlMat, r.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // Scale
        mdlMat = glm::scale(mdlMat, s);
    }

    virtual void respawn() {
        mdlMat = glm::mat4(1.0f);
    }
};

#endif //ENTITY_H
