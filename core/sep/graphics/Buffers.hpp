//
// Created by down1 on 29.12.2025.
//

#ifndef INC_2G43S_BUFFER_H
#define INC_2G43S_BUFFER_H

#include <vector>
#include <vulkan/vulkan_core.h>

struct DrawCommandsBufferObject;
struct ModelBus;

struct Buffers {
    static void createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    static void copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size);

    // Creating various buffers
    // Main
    static void createCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers, const int& MAX_FRAMES_IN_FLIGHT);

    static void createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, const ModelBus& mdlBus);

    static void createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer &indexBuffer, VkDeviceMemory& indexBufferMemory, const ModelBus& mdlBus);

    // Generic
    static void createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& Buffers, std::vector<VkDeviceMemory>& BuffersMemory, std::vector<void*>& BuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);

    static void createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags);

    static void createUniformBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& uniformBuffers, std::vector<VkDeviceMemory>& uniformBuffersMemory, std::vector<void*>& uniformBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT);

    static void createUniformCullingBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& uniformCullingBuffers, std::vector<VkDeviceMemory>& uniformCullingBuffersMemory, std::vector<void*>& uniformCullingBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT);

    static void createAtomicCounterBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& atomicCounterBuffers, std::vector<VkDeviceMemory>& atomicCounterBuffersMemory, std::vector<void*>& atomicCounterBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT);

    // Matrices
    static void createModelBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& modelBuffers, std::vector<VkDeviceMemory>& modelBuffersMemory, std::vector<void*>& modelBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus);

    static void createModelDataBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& modelDataBuffers, std::vector<VkDeviceMemory>& modelDataBuffersMemory, std::vector<void*>& modelDataBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus);

    // Culling
    static void createVisibleIndicesBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& visibleIndicesBuffers, std::vector<VkDeviceMemory>& visibleIndicesBuffersMemory, std::vector<void*>& visibleIndicesBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus);

    static void createModelCullingBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& modelCullingBuffers, VkDeviceMemory& modelCullingBuffersMemory, void*& modelCullingBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus);

    static void updateOrCreateDrawCommandsBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& drawCommandsBuffer, VkDeviceMemory& drawCommandsBufferMemory, ModelBus& mdlBus, void*& drawCommandsBufferMapped, const std::vector<VkDrawIndexedIndirectCommand>& drawCommands, DrawCommandsBufferObject& dc);

    static void createDrawCommandsBuffer(const VkDevice &device, const VkPhysicalDevice &physicalDevice, VkBuffer &drawCommandsBuffer, VkDeviceMemory &drawCommandsBufferMemory, ModelBus &mdlBus, void *&drawCommandsBufferMapped, DrawCommandsBufferObject& dc);

    static void updateDrawCommandsBuffer(const VkBuffer& drawCommandsBuffer, void*& drawCommandsBufferMapped, const std::vector<VkDrawIndexedIndirectCommand>& drawCommands);
};


#endif //INC_2G43S_BUFFER_H