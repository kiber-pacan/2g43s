#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.h"
#include "../entity/Entity.h"

struct ModelInstance : Entity {
    std::shared_ptr<ParsedModel> mdl;

    ModelInstance(std::shared_ptr<ParsedModel> m) : mdl(std::move(m)) {}

    ModelInstance(std::shared_ptr<ParsedModel> m, glm::vec3 pos) {
        this->mdl = std::move(m);
        this->pos = pos;
    }
};

#endif //INSTANCE_H
