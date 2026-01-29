//
// Created by down1 on 29.12.2025.
//

#include "Buffers.hpp"

#include "Vertex.hpp"
#include "ModelEntityManager.hpp"

// Generic
void Buffers::createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo allocFlagsInfo{};
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Helper::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);
    allocInfo.pNext = &allocFlagsInfo;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Buffers::copyBuffer(const VkDevice& device, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, const VkDeviceSize& size) {
    const VkCommandBuffer& commandBuffer = Command::beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    Command::endSingleTimeCommands(device, commandBuffer, commandPool, graphicsQueue);
}

void Buffers::createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    buffers.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, buffers[i], buffersMemory[i]);

        vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);
    }
}

void Buffers::createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<uint64_t>& constants, std::vector<VkBuffer>& buffers, std::vector<VkDeviceMemory>& buffersMemory, std::vector<void*>& buffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    buffers.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    constants.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, buffers[i], buffersMemory[i]);
        vkMapMemory(device, buffersMemory[i], 0, bufferSize, 0, &buffersMapped[i]);

        VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
        address_info.buffer = buffers[i];

        constants[i] = vkGetBufferDeviceAddress(device, &address_info);
    }
}

void Buffers::createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, buffer, bufferMemory);

    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &bufferMapped);
}

void Buffers::createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, uint64_t& constant, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, buffer, bufferMemory);

    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &bufferMapped);

    VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
    address_info.buffer = buffer;

    constant = vkGetBufferDeviceAddress(device, &address_info);
}



// Main
void Buffers::createCommandBuffer(const VkDevice& device, const VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffers, const int& MAX_FRAMES_IN_FLIGHT) {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Buffers::createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, const ModelEntityManager& mdlBus) {
    const VkDeviceSize bufferSize = mdlBus.getVertexBufferSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    const std::vector<Vertex> vertices = mdlBus.getAllVertices();
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);

    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Buffers::createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer &indexBuffer, VkDeviceMemory& indexBufferMemory, const ModelEntityManager& mem) {
    const VkDeviceSize bufferSize = mem.getIndexBufferSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    const std::vector<u_int32_t> indices = mem.getAllIndices();
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
