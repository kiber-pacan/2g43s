#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.hpp"
#include "../entity/Entity.hpp"

struct ModelInstance final : Entity {
    std::weak_ptr<ParsedModel> mdl;

    explicit ModelInstance(const std::shared_ptr<ParsedModel>& mdl,
                           glm::vec4 pos = glm::vec4(0.0f),
                           glm::vec4 rot = glm::vec4(0.0f),
                           glm::vec4 scl = glm::vec4(1.0f))
        : Entity(pos, rot, scl), mdl(mdl) {}
};

#endif //INSTANCE_H
