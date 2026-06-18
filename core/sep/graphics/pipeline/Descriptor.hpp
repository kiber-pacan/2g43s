//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_DESCRIPTOR_H
#define INC_2G43S_DESCRIPTOR_H

#include <vector>
#include <vulkan/vulkan_core.h>

struct ModelEntityManager;

struct Descriptor {
    // Graphics
    static void createGraphicsDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout& graphicsDescriptorSetLayout
        );

    static void createGraphicsDescriptorPool(
        VkDevice device,
        VkDescriptorPool& graphicsDescriptorPool,
        size_t MAX_FRAMES_IN_FLIGHT
        );

    static void createGraphicsDescriptorSets(
        VkDevice device,
        VkDescriptorSetLayout& graphicsDescriptorSetLayout, VkDescriptorPool& graphicsDescriptorPool, std::vector<VkDescriptorSet>& graphicsDescriptorSets,
        VkSampler textureSampler, VkImageView textureImageView,
        const ModelEntityManager &modelEntityManager, size_t MAX_FRAMES_IN_FLIGHT
        );


    // Postprocess
    static void createPostprocessDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout& postProcessingDescriptorSetLayout
        );

    static void createPostprocessDescriptorPool(
        VkDevice device,
        VkDescriptorPool& postProcessingDescriptorPool,
        size_t MAX_FRAMES_IN_FLIGHT
        );

    static void createPostprocessDescriptorSets(
        VkDevice device,
        VkDescriptorSetLayout& postProcessingDescriptorSetLayout, VkDescriptorPool& postProcessingDescriptorPool, std::vector<VkDescriptorSet>& postProcessingDescriptorSets,
        const std::vector<VkImageView> &offscreenImageViews, VkImageView depthImageView, VkSampler textureSampler,
        const std::vector<VkBuffer> &uniformPostprocessingBuffers, size_t MAX_FRAMES_IN_FLIGHT);

    static void updatePostprocessDescriptorSets(
        VkDevice device,
        const std::vector<VkDescriptorSet>& postProcessingDescriptorSets,
        const std::vector<VkImageView> &offscreenImageViews, VkImageView imageView, VkSampler textureSampler,
        size_t MAX_FRAMES_IN_FLIGHT
        );
};


#endif //INC_2G43S_DESCRIPTOR_H