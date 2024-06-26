#ifndef QUEUE_H
#define QUEUE_H

#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#include <optional>
#include <iostream>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

struct queue {
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice& device) {
        QueueFamilyIndices indices;

        //Get queue families count
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        //Get queue families
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            i++;
        }

        return indices;
    }
};

#endif //QUEUE_H
