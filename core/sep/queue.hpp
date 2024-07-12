#ifndef QUEUE_H
#define QUEUE_H

#include <vulkan/vulkan.h>
#include <optional>

//Queue families
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct queue {
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice &device, VkSurfaceKHR &surface) {
        QueueFamilyIndices indices;

        // Get queue count
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // Get queue families
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // Comparing bits and then setting index of graphicsFamily
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            // Checking present support by queueFamily
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            // Setting index
            if (presentSupport) {
                indices.presentFamily = i;
            }

            // If graphics and present family != null break for loop
            if (indices.isComplete()) {
                break;
            }
            i++;
        }

        return indices;
    }
};

#endif // QUEUE_H
