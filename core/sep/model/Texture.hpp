//
// Created by down1 on 09.01.2026.
//

#ifndef INC_2G43S_TEXTURE_H
#define INC_2G43S_TEXTURE_H
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

struct Texture {
    stbi_uc* pixels;
    int texWidth, texHeight, texChannels;
    size_t index;
    size_t indexOffset;

    // LATE LOADING STUFF
    VkImage textureImage{};
    VkImageView textureImageView{};
    VkDeviceMemory textureImageMemory{};
};

#endif //INC_2G43S_TEXTURE_H