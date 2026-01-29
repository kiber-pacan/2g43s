#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.hpp"
#include "Entity.hpp"

struct ModelInstance final : Entity {
    std::weak_ptr<ParsedModel> mdl;
    ModelInstance() = default;

    explicit ModelInstance(const std::shared_ptr<ParsedModel>& mdl,
                           glm::vec4 pos = glm::vec4(0.0f),
                           glm::vec4 rot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                           glm::vec4 scl = glm::vec4(1.0f))
        : mdl(mdl) {
        this->pos = pos;
        this->rot = rot;
        this->scl = scl;
    }
};

#endif //INSTANCE_H
