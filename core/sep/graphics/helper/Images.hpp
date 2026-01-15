//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_IMAGE_H
#define INC_2G43S_IMAGE_H

#include <cstddef>
#include <vulkan/vulkan_core.h>

#include "Texture.hpp"

struct Images {
    // Image
    static void createImage(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    static VkImageView createImageView(const VkDevice& device, const VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags);

    // Texture
    static void createTextureImage(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory, VkImage& textureImage, VkDeviceMemory& textureImageMemory);

    static void createTextureImage(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkPhysicalDevice& physicalDevice, VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory, Texture& texture);

    static void createTextureImageView(const VkDevice& device, const VkImage& textureImage, VkImageView& textureImageView);

    static void createTextureSampler(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkSampler& textureSampler);

    // Helper
    static void copyBufferToImage(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkBuffer& buffer, const VkImage& image, const uint32_t& width, const uint32_t& height);

    static void transitionImageLayout(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkImage& image, const VkImageLayout& oldLayout, const VkImageLayout& newLayout);
};


#endif //INC_2G43S_IMAGE_H