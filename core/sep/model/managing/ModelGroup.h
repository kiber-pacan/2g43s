//
// Created by down1 on 29.01.2026.
//

#ifndef INC_2G43S_MODELGROUP_H
#define INC_2G43S_MODELGROUP_H
#include <memory>
#include <vector>

#include "ModelInstance.hpp"

struct ModelGroup {
    std::shared_ptr<ParsedModel> model{};
    std::vector<ModelInstance> instances{};

    ModelGroup() = default;

    ModelGroup(const std::shared_ptr<ParsedModel> &model, const std::vector<ModelInstance> &instances) : model(model), instances(instances) {}

    explicit ModelGroup(const std::shared_ptr<ParsedModel> &model) : model(model) {}
};
#endif //INC_2G43S_MODELGROUP_H