#ifndef ENTITY_H
#define ENTITY_H
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


struct Entity {
    virtual ~Entity() = default;

    glm::vec4 pos{};
    glm::vec4 rot{};
    glm::vec4 scl{};

    explicit Entity(
        const glm::vec4 p = glm::vec4(0.0f),
        const glm::vec4 r = glm::vec4(0.0f),
        const glm::vec4 s = glm::vec4(1.0f)
    ) : pos(p), rot(r), scl(s) {}

    virtual void respawn() {
        pos = glm::vec4(0.0f);
        rot = glm::vec4(0.0f);
        scl = glm::vec4(1.0f);
    }
};

#endif //ENTITY_H
