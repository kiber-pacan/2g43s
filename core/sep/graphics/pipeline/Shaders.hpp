//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_SHADER_H
#define INC_2G43S_SHADER_H

#include <vector>
#include <vulkan/vulkan_core.h>


struct Shaders {
    static VkShaderModule createShaderModule(const std::vector<char>& code, const VkDevice& device);
};


#endif //INC_2G43S_SHADER_H