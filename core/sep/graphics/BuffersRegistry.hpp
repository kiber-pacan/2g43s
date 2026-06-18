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

struct BuffersRegistry {
    // Generic
    static void createBuffer(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory
        );

    static void copyBuffer(
        VkDevice device,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        VkBuffer srcBuffer, VkBuffer dstBuffer,
        VkDeviceSize size
        );

    static void createGenericBuffers(
        VkDevice device, VkPhysicalDevice physicalDevice,
        std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped,
        size_t MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags
        );

    static void createGenericBuffers(
        VkDevice device, VkPhysicalDevice physicalDevice,
        std::vector<uint64_t>& constants,
        std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped,
        size_t MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags
        );


    static void createGenericBuffer(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped,
        VkDeviceSize bufferSize, int usageFlags, int memoryFlags
        );

    static void createGenericBuffer(
        VkDevice device, VkPhysicalDevice physicalDevice,
        uint64_t& constant,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped,
        VkDeviceSize bufferSize, int usageFlags, int memoryFlags
        );


    // Main
    static void createCommandBuffer(
        VkDevice device,
        VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers,
        size_t MAX_FRAMES_IN_FLIGHT
        );

    static void createVertexBuffer(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory,
        const ModelEntityManager& mem);

    static void createIndexBuffer(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory,
        const ModelEntityManager& mem
        );
};


#endif //INC_2G43S_BUFFER_H