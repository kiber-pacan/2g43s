#ifndef INSTANCE_H
#define INSTANCE_H
#include <utility>

#include "ParsedModel.h"
#include "../entity/Entity.h"

struct ModelInstance : Entity {
    std::shared_ptr<ParsedModel> mdl;

    explicit ModelInstance(std::shared_ptr<ParsedModel> m) : mdl(std::move(m)) {}
};

#endif //INSTANCE_H
