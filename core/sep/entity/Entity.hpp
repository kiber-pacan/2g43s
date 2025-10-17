#ifndef ENTITY_H
#define ENTITY_H
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


struct Entity {
    virtual ~Entity() = default;
    Entity() = default;

    glm::vec4 pos{};
    glm::vec4 rot{};
    glm::vec4 scl{};

    explicit Entity(
        const glm::vec4& pos,
        const glm::vec4& rot,
        const glm::vec4& scl
    ) : pos(pos), rot(rot), scl(scl) {}

    virtual void respawn() {
        pos = glm::vec4(0.0f);
        rot = glm::vec4(0.0f);
        scl = glm::vec4(1.0f);
    }
};

#endif //ENTITY_H
