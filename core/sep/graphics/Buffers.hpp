//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_BUFFER_H
#define INC_2G43S_BUFFER_H

#include <vector>
#include <vulkan/vulkan_core.h>

#include <stdexcept>

#include "Command.hpp"
#include "Helper.hpp"


struct ModelEntityManager;

struct Buffers {
    // Generic
    static void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    static void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size);

    static void createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);

    static void createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<uint64_t>& constants, std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);


    static void createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);

    static void createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint64_t& constant, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);


    // Main
    static void createCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers, const int& MAX_FRAMES_IN_FLIGHT);

    static void createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, const ModelEntityManager& mem);

    static void createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer &indexBuffer, VkDeviceMemory& indexBufferMemory, const ModelEntityManager& mem);
};


#endif //INC_2G43S_BUFFER_H