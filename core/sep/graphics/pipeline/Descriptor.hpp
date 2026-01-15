//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_DESCRIPTOR_H
#define INC_2G43S_DESCRIPTOR_H

#include <vector>
#include <vulkan/vulkan_core.h>

#include "ModelBus.hpp"


struct Descriptor {
    // Graphics
    static void createGraphicsDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& graphicsDescriptorSetLayout);

    static void createGraphicsDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& graphicsDescriptorPool);

    static void createGraphicsDescriptorSets(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout& graphicsDescriptorSetLayout, const VkDescriptorPool& graphicsDescriptorPool, std::vector<VkDescriptorSet>& graphicsDescriptorSets, const std::vector<VkBuffer>& uniformBuffers, const VkSampler& textureSampler, const std::vector<VkBuffer>& modelBuffers, const std::vector<VkBuffer>& visibleIndicesBuffers, const ModelBus& mdlBus, const VkImageView& textureImageView, const VkBuffer& textureIndexBuffer, const VkBuffer& textureIndexOffsetBuffer);


    // Matrix
    static void createMatrixComputeDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& matrixComputeDescriptorSetLayout);

    static void createMatrixComputeDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& matrixComputeDescriptorPool);

    static void createMatrixComputeDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout &matrixComputeDescriptorSetLayout, const VkDescriptorPool &matrixComputeDescriptorPool, std::vector<VkDescriptorSet> &matrixComputeDescriptorSets, const std::vector<VkBuffer> &modelBuffers, const ModelBus &mdlBus, const std::vector<VkBuffer> &modelDataBuffers);


    // Culling
    static void createCullingComputeDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& cullingComputeDescriptorSetLayout);

    static void createCullingComputeDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& cullingComputeDescriptorPool);

    static void createCullingComputeDescriptorSets(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout& cullingComputeDescriptorSetLayout,
                                                   const VkDescriptorPool& cullingComputeDescriptorPool, std::vector<VkDescriptorSet>& cullingComputeDescriptorSets,
                                                   const ModelBus& mdlBus, const VkBuffer&  drawCommandsBuffer, const std::vector<VkBuffer>& uniformCullingBuffers,
                                                   const std::vector<VkBuffer>& visibleIndicesBuffers, const VkBuffer& modelCullingBuffer,
                                                   const std::vector<VkBuffer>& atomicCounterBuffers);


    // Postprocess
    static void createPostprocessDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& postProcessingDescriptorSetLayout);

    static void createPostprocessDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& postProcessingDescriptorPool);

    static void createPostprocessDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout &postProcessingDescriptorSetLayout, const
                                                VkDescriptorPool &postProcessingDescriptorPool, std::vector<VkDescriptorSet> &postProcessingDescriptorSets, const std::
                                                vector<VkImageView> &offscreenImageViews, const VkImageView& depthImage, const VkSampler &textureSampler, const std::vector<VkBuffer> &
                                                uniformPostprocessingBuffers);

    static void updatePostprocessDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const std::vector<VkDescriptorSet> &postProcessingDescriptorSets, const std::vector<VkImageView> &offscreenImageViews, const VkImageView& ImageView, const VkSampler &textureSampler);

};


#endif //INC_2G43S_DESCRIPTOR_H