//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_HELPER_H
#define INC_2G43S_HELPER_H

#include <vector>
#include <vulkan/vulkan_core.h>

struct Helper {
    static void createSyncObjects(const VkDevice& device, std::vector<VkSemaphore>& imageAvailableSemaphores, std::vector<VkSemaphore>& renderFinishedSemaphores, std::vector<VkFence>& inFlightFences, const int MAX_FRAMES_IN_FLIGHT);

    static void createDepthResources(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkImage& depthImage, VkDeviceMemory& depthImageMemory, VkImageView& depthImageView, const VkExtent2D& swapchainExtent);

    // Choosing suitable memory type
    static uint32_t findMemoryType( const uint32_t& typeFilter, VkMemoryPropertyFlags properties, const VkPhysicalDevice& physicalDevice);

    // Finding supported image format
    static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, const VkPhysicalDevice& physicalDevice);

    static VkFormat findDepthFormat(const VkPhysicalDevice& physicalDevice);

    static bool hasStencilComponent(VkFormat format);
};


#endif //INC_2G43S_HELPER_H