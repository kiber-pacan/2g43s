//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_PIPELINE_H
#define INC_2G43S_PIPELINE_H

#include <vulkan/vulkan_core.h>
#include <string>

struct PipelineCreation {
    static void createGraphicsPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, VkPipelineLayout& pipelineLayout, VkPipeline& graphicsPipeline, VkDescriptorSetLayout& descriptorSetLayout, VkFormat& swapchainImageFormat);

    static void createMatrixComputePipeline(const VkDevice& device, const VkDescriptorSetLayout& matrixComputeDescriptorSetLayout, VkPipelineLayout& matrixComputePipelineLayout, VkPipeline& matrixComputePipeline);

    static void createCullingComputePipeline(const VkDevice& device, const VkDescriptorSetLayout& cullingComputeDescriptorSetLayout, VkPipelineLayout& cullingComputePipelineLayout, VkPipeline& cullingComputePipeline);

    static void createPostprocessPipelineLayout(const VkDevice &device, VkPipelineLayout &pipelineLayout, const VkDescriptorSetLayout &descriptorSetLayout);

    static void createPostprocessPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, VkPipelineLayout& pipelineLayout, VkPipeline& postprocessPipeline, VkDescriptorSetLayout& descriptorSetLayout, VkFormat& swapchainImageFormat, const std::string filename);
};


#endif //INC_2G43S_PIPELINE_H