#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.hpp"
#include "../entity/Entity.hpp"

struct ModelInstance final : Entity {
    std::shared_ptr<ParsedModel> mdl;

    explicit ModelInstance(std::shared_ptr<ParsedModel> m = nullptr,
                           glm::vec4 p = glm::vec4(0.0f),
                           glm::vec4 r = glm::vec4(0.0f),
                           glm::vec4 s = glm::vec4(1.0f))
        : Entity(p, r, s), mdl(std::move(m)) {}
};

#endif //INSTANCE_H
