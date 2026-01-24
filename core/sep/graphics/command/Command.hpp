//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_COMMAND_H
#define INC_2G43S_COMMAND_H

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "Queue.hpp"


struct Command {
    // Command shi
    static void createCommandPool(const VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkSurfaceKHR& surface);

    static VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool);

    static void endSingleTimeCommands(const VkDevice& device, const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
};


#endif //INC_2G43S_COMMAND_H