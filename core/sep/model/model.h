#ifndef MODEL_H
#define MODEL_H
#include "../graphics/graphics.hpp"
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

struct mdl {
    mdl();

    mdl(const char* path) {

    }


    std::vector<graphics::Vertex> vertices{};
    std::vector<uint16_t> indices{};
    VkImage textureImage{};
};

#endif //MODEL_H
