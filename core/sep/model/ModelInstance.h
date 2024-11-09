#ifndef INSTANCE_H
#define INSTANCE_H
#include "ParsedModel.h"
#include "../entity/Entity.h"

struct mdlInstance : Entity {
    ParsedModel model{};
};

#endif //INSTANCE_H
