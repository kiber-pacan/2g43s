//
// Created by down1 on 09.01.2026.
//

#ifndef INC_2G43S_TEXTURE_H
#define INC_2G43S_TEXTURE_H
#include <vulkan/vulkan_core.h>


struct Texture {
    uint8_t* pixels{};

    uint32_t texWidth{};
    uint32_t texHeight{};
    uint32_t imageSize{};

    // Texture indexing in fragment shader
    size_t index{};
    size_t indexOffset{};

    // Vulkan
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImage textureImage = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;

    void deleteImage() const {
        delete[] pixels;
    }
};

#endif //INC_2G43S_TEXTURE_H