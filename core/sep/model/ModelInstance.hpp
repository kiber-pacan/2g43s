#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.hpp"
#include "Entity.hpp"

struct ModelInstance final : Entity {
    std::weak_ptr<ParsedModel> mdl;
    glm::vec4 sfr{};

    ModelInstance() = default;

    explicit ModelInstance(const std::shared_ptr<ParsedModel>& mdl,
                           glm::vec4 pos = glm::vec4(0.0f),
                           glm::vec4 rot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                           glm::vec4 scl = glm::vec4(1.0f))
        : mdl(mdl) {
        this->pos = pos;
        this->rot = rot;
        this->scl = scl;
        this->sfr = glm::vec4(pos.x + mdl->sphere.x, pos.y + mdl->sphere.y, pos.z + mdl->sphere.z, mdl->sphere.w);
    }
};

#endif //INSTANCE_H
