//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_COMMAND_H
#define INC_2G43S_COMMAND_H

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Engine.hpp"
#include "model/bus/ModelBus.hpp"
#include "util/Color.hpp"
#include "util/Queue.hpp"


struct Command {
    // Command shi
    static void createCommandPool(const VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkSurfaceKHR& surface);


    static void recordCommandBuffer(
    VkDevice& device,
    VkPhysicalDevice& physicalDevice,
    VkCommandBuffer& commandBuffer,
    uint32_t imageIndex,
    VkExtent2D& swapchainExtent,
    VkBuffer& vertexBuffer,
    VkBuffer& indexBuffer,
    VkPipeline& graphicsPipeline,
    VkPipeline& matrixComputePipeline,
    VkPipeline& cullingComputePipeline,
    VkPipeline& postprocessPipeline,
    VkPipelineLayout& graphicsPipelineLayout,
    VkPipelineLayout& matrixComputePipelineLayout,
    VkPipelineLayout& cullingComputePipelineLayout,
    VkPipelineLayout& postprocessPipelineLayout,
    std::vector<VkDescriptorSet>& graphicsDescriptorSets,
    std::vector<VkDescriptorSet>& matrixComputeDescriptorSets,
    std::vector<VkDescriptorSet>& cullingComputeDescriptorSets,
    std::vector<VkDescriptorSet>& postprocessDescriptorSets,
    uint32_t& currentFrame,
    Color clear_color,
    ModelBus& mdlBus,
    std::vector<VkBuffer>& modelDataBuffers,
    VkBuffer& drawCommandsBuffer,
    int MAX_FRAMES_IN_FLIGHT,
    bool& matrixDirty,
    std::vector<VkBuffer>& atomicCounterBuffers,
    std::vector<VkBuffer>& visibleIndicesBuffers,
    const std::function<void(VkCommandBuffer&)>& drawImGui,
    const std::vector<VkImageView>& swapchainImageViews,
    VkImageView& depthImageView,
    VkImage& depthImage,
    std::vector<VkImage>& swapchainImages,
    std::vector<VkImageView>& offscreenImageViews,
    std::vector<VkImage>& offscreenImages,
    bool depth
    );

    static VkCommandBuffer beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool);

    static void endSingleTimeCommands(const VkDevice& device, const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
};


#endif //INC_2G43S_COMMAND_H