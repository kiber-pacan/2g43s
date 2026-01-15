//
// Created by down1 on 29.12.2025.
//

#include "Buffers.hpp"

#include <ranges>
#include <stdexcept>

#include "command/Command.hpp"
#include "DrawCommandsBufferObject.hpp"
#include "Helper.hpp"
#include "../buffers/main/uniform/UniformBufferObject.hpp"
#include "UniformCullingBufferObject.hpp"


void Buffers::createBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Helper::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

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

// Creating various buffers
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

void Buffers::createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, const ModelBus& mdlBus) {
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

void Buffers::createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue, VkBuffer &indexBuffer, VkDeviceMemory& indexBufferMemory, const ModelBus& mdlBus) {
    const VkDeviceSize bufferSize = mdlBus.getIndexBufferSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    const std::vector<u_int32_t> indices = mdlBus.getAllIndices();
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Generic
void Buffers::createGenericBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& Buffers, std::vector<VkDeviceMemory>& BuffersMemory, std::vector<void*>& BuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    Buffers.resize(MAX_FRAMES_IN_FLIGHT);
    BuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    BuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, Buffers[i], BuffersMemory[i]);

        vkMapMemory(device, BuffersMemory[i], 0, bufferSize, 0, &BuffersMapped[i]);
    }
}

void Buffers::createGenericBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void*& bufferMapped, VkDeviceSize bufferSize, int usageFlags, int memoryFlags) {
    createBuffer(device, physicalDevice, bufferSize, usageFlags, memoryFlags, buffer, bufferMemory);

    vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &bufferMapped);

}

void Buffers::createUniformBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& uniformBuffers, std::vector<VkDeviceMemory>& uniformBuffersMemory, std::vector<void*>& uniformBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT) {
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void Buffers::createUniformCullingBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& uniformCullingBuffers, std::vector<VkDeviceMemory>& uniformCullingBuffersMemory, std::vector<void*>& uniformCullingBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT) {
    uniformCullingBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformCullingBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformCullingBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        constexpr VkDeviceSize bufferSize = sizeof(UniformCullingBufferObject);
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformCullingBuffers[i], uniformCullingBuffersMemory[i]);

        vkMapMemory(device, uniformCullingBuffersMemory[i], 0, bufferSize, 0, &uniformCullingBuffersMapped[i]);
    }
}

void Buffers::createAtomicCounterBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& atomicCounterBuffers, std::vector<VkDeviceMemory>& atomicCounterBuffersMemory, std::vector<void*>& atomicCounterBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT) {
    atomicCounterBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    atomicCounterBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    atomicCounterBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        constexpr VkDeviceSize bufferSize = sizeof(uint32_t);
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, atomicCounterBuffers[i], atomicCounterBuffersMemory[i]);

        vkMapMemory(device, atomicCounterBuffersMemory[i], 0, bufferSize, 0, &atomicCounterBuffersMapped[i]);
    }
}

// Matrices
void Buffers::createModelBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& modelBuffers, std::vector<VkDeviceMemory>& modelBuffersMemory, std::vector<void*>& modelBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus) {
    const VkDeviceSize bufferSize = mdlBus.getModelBufferSize();

    modelBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    modelBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    modelBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, modelBuffers[i], modelBuffersMemory[i]);

        vkMapMemory(device, modelBuffersMemory[i], 0, bufferSize, 0, &modelBuffersMapped[i]);
    }
}

void Buffers::createModelDataBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& modelDataBuffers, std::vector<VkDeviceMemory>& modelDataBuffersMemory, std::vector<void*>& modelDataBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus) {
    const VkDeviceSize bufferSize = mdlBus.getModelDataBufferSize();

    modelDataBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    modelDataBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    modelDataBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, modelDataBuffers[i], modelDataBuffersMemory[i]);

        vkMapMemory(device, modelDataBuffersMemory[i], 0, bufferSize, 0, &modelDataBuffersMapped[i]);
    }
}

// Culling
void Buffers::createVisibleIndicesBuffers(const VkDevice& device, const VkPhysicalDevice& physicalDevice, std::vector<VkBuffer>& visibleIndicesBuffers, std::vector<VkDeviceMemory>& visibleIndicesBuffersMemory, std::vector<void*>& visibleIndicesBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus) {
    const VkDeviceSize bufferSize = sizeof(uint32_t) * mdlBus.getTotalInstanceCount();

    visibleIndicesBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    visibleIndicesBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    visibleIndicesBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, visibleIndicesBuffers[i], visibleIndicesBuffersMemory[i]);

        vkMapMemory(device, visibleIndicesBuffersMemory[i], 0, bufferSize, 0, &visibleIndicesBuffersMapped[i]);
    }
}

void Buffers::createModelCullingBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& modelCullingBuffers, VkDeviceMemory& modelCullingBuffersMemory, void*& modelCullingBuffersMapped, const int& MAX_FRAMES_IN_FLIGHT, const ModelBus& mdlBus) {
    const VkDeviceSize bufferSize = sizeof(CullingData) * mdlBus.getTotalInstanceCount();


    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, modelCullingBuffers, modelCullingBuffersMemory);

    vkMapMemory(device, modelCullingBuffersMemory, 0, bufferSize, 0, &modelCullingBuffersMapped);
}

void Buffers::updateOrCreateDrawCommandsBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkBuffer& drawCommandsBuffer, VkDeviceMemory& drawCommandsBufferMemory, ModelBus& mdlBus, void*& drawCommandsBufferMapped, const std::vector<VkDrawIndexedIndirectCommand>& drawCommands, DrawCommandsBufferObject& dc) {
    if (drawCommandsBufferMapped == nullptr) {
        createDrawCommandsBuffer(device, physicalDevice, drawCommandsBuffer, drawCommandsBufferMemory, mdlBus, drawCommandsBufferMapped, dc);
    } else {
        updateDrawCommandsBuffer(drawCommandsBuffer, drawCommandsBufferMapped, drawCommands);
    }
}

void Buffers::createDrawCommandsBuffer(const VkDevice &device, const VkPhysicalDevice &physicalDevice, VkBuffer &drawCommandsBuffer, VkDeviceMemory &drawCommandsBufferMemory, ModelBus &mdlBus, void *&drawCommandsBufferMapped, DrawCommandsBufferObject& dc) {
    dc.commands.clear();

    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;

    for (const auto& group : std::views::values(mdlBus.groups_map)) {
        VkDrawIndexedIndirectCommand command{};

        const size_t instanceCount = group.instances.size();
        const uint32_t indexCount = ModelBus::getIndexCount(group.model);

        command.firstInstance = firstInstance;
        command.instanceCount = instanceCount;

        command.firstIndex = firstIndex;
        command.indexCount = indexCount;
        command.vertexOffset = vertexOffset;

        firstIndex += indexCount;
        vertexOffset += ModelBus::getVertexCount(group.model);
        firstInstance += instanceCount;
        dc.commands.emplace_back(command);
    }

    //mdlBus.dirtyCommands = false;

    const VkDeviceSize bufferSize = sizeof(VkDrawIndexedIndirectCommand) * dc.commands.size();

    if (drawCommandsBufferMapped != nullptr) {
        vkUnmapMemory(device, drawCommandsBufferMemory);
        vkDestroyBuffer(device, drawCommandsBuffer, nullptr);
        vkFreeMemory(device, drawCommandsBufferMemory, nullptr);
    }

    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, drawCommandsBuffer, drawCommandsBufferMemory);

    if (vkMapMemory(device, drawCommandsBufferMemory, 0, bufferSize, 0, &drawCommandsBufferMapped) != VK_SUCCESS) {
        throw std::runtime_error("failed to map draw command buffer memory!");
    }

    memcpy(drawCommandsBufferMapped, dc.commands.data(), bufferSize);
}

void Buffers::updateDrawCommandsBuffer(const VkBuffer& drawCommandsBuffer, void*& drawCommandsBufferMapped, const std::vector<VkDrawIndexedIndirectCommand>& drawCommands) {
    if (drawCommands.empty()) return;

    const VkDeviceSize bufferSize = sizeof(drawCommands[0]) * drawCommands.size();

    std::memcpy(drawCommandsBufferMapped, drawCommands.data(), bufferSize);
}