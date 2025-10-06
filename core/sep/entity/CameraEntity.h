#ifndef CAMERAENTITY_H
#define CAMERAENTITY_H
#include <utility>

#include "glm/vec3.hpp"

struct CameraEntity {
    virtual ~CameraEntity() = default;

    std::string name;

    glm::vec3 pos{};
    glm::vec3 look{};
    glm::vec3 size{};

    explicit CameraEntity(
        glm::vec3 p = glm::vec3(0.0f),
        glm::vec3 l = glm::vec3(0.0f),
        glm::vec3 s = glm::vec3(1.0f),
        std::string n = ""
        ) : name(std::move(n)), pos(p), look(l), size(s) {}

    virtual void respawn() {
        pos = glm::vec3(0);
        look = glm::vec3(0);
        size = glm::vec3(1);
    }
};

#endif //CAMERAENTITY_H
