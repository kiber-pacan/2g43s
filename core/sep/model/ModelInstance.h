#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.h"
#include "../entity/Entity.h"

struct ModelInstance final : Entity {
    std::shared_ptr<ParsedModel> mdl;

    explicit ModelInstance(std::shared_ptr<ParsedModel> m) : mdl(std::move(m)) {}

    explicit ModelInstance(std::shared_ptr<ParsedModel> m,
                  glm::vec3 p = glm::vec3(0.0f),
                  glm::vec3 r = glm::vec3(0.0f),
                  glm::vec3 s = glm::vec3(1.0f)
                  ) : Entity(p, r, s), mdl(std::move(m)) {

    }
};

#endif //INSTANCE_H
