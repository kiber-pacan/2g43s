#ifndef INC_2G43S_COMMANDS_H
#define INC_2G43S_COMMANDS_H

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_core.h>

struct DrawCommandsBufferObject {
    std::vector<VkDrawIndexedIndirectCommand> commands{};
};

#endif //INC_2G43S_COMMANDS_H