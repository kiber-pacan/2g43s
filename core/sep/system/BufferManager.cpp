//
// Created by down1 on 24.02.2026.
//


#include "BufferManager.hpp"

#include <cstring>


#include "Camera.hpp"
#include "DeltaManager.hpp"
#include "ModelEntityManager.hpp"
#include "glmMath.h"
#include "SwapchainManager.hpp"

#pragma region Update
// Generic
void BufferManager::updateUniformBuffer(uint32_t currentFrame) {
    const glm::mat4 view = glm::lookAt(camera->pos + glm::vec3(0), camera->pos + camera->look, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 proj = glm::perspective(camera->fov,  static_cast<float>(swapchainManager->swapchainExtent.width) / static_cast<float>(swapchainManager->swapchainExtent.height), 0.1f, 4096.0f);
    proj[1][1] *= -1;

    uniformBufferObject.view = view;
    uniformBufferObject.proj = proj;
    uniformBufferObject.count = modelEntityManager->getTotalModelCount();

    uniformModelBufferObject.view = view;
    uniformModelBufferObject.proj = proj;
    uniformModelBufferObject.count = modelEntityManager->getTotalInstanceCount();

    memcpy(uniformBuffersMapped[currentFrame], &uniformBufferObject, sizeof(uniformBufferObject));
    memcpy(uniformMatrixBuffersMapped[currentFrame], &uniformModelBufferObject, sizeof(uniformModelBufferObject));
}

void BufferManager::updateUniformPostprocessingBuffer(uint32_t currentFrame, const DeltaManager& delta) {
    uniformPostprocessingBufferObject.time += static_cast<float>(delta.deltaTime);
    uniformPostprocessingBufferObject.resolution = glm::vec2(swapchainManager->swapchainExtent.width, swapchainManager->swapchainExtent.height);
    memcpy(uniformPostprocessingBuffersMapped[currentFrame], &uniformPostprocessingBufferObject, sizeof(uniformPostprocessingBufferObject));
}

void BufferManager::normalizePlane(glm::vec4& plane) {
    const float length = glm::length(glm::vec3(plane));
    if (length > 0.0f) {
        plane /= length;
    }
}

glm::vec4 BufferManager::extractPlane(glm::mat4 viewProjection, const int a, const float sign) {
    glm::vec4 plane(
        viewProjection[0][3] - sign * viewProjection[0][a],
        viewProjection[1][3] - sign * viewProjection[1][a],
        viewProjection[2][3] - sign * viewProjection[2][a],
        viewProjection[3][3] - sign * viewProjection[3][a]
    );

    const float length = glm::length(glm::vec3(plane));
    if (length > 0.0f) plane /= length;

    return plane;
}

void BufferManager::updateCullingUniformBuffer(uint32_t currentFrame) {
    uniformCullingBufferObject.totalObjects = modelEntityManager->getTotalInstanceCount();

    const glm::mat4 view = glm::lookAt(camera->pos, camera->pos + camera->look, glm::vec3(0.0f, 0.0f, 1.0f)); // z-up
    glm::mat4 proj = glm::perspective(camera->fov, static_cast<float>(swapchainManager->swapchainExtent.width) / static_cast<float>(swapchainManager->swapchainExtent.height), 0.1f, 4096.0f);
    proj[1][1] *= -1;

    const glm::mat4 viewProjection = proj * view;

    uniformCullingBufferObject.planes[0] = extractPlane(viewProjection, 0, +1); // right
    uniformCullingBufferObject.planes[1] = extractPlane(viewProjection, 0, -1); // left
    uniformCullingBufferObject.planes[2] = extractPlane(viewProjection, 1, +1); // top
    uniformCullingBufferObject.planes[3] = extractPlane(viewProjection, 1, -1); // bottom
    uniformCullingBufferObject.planes[4] = extractPlane(viewProjection, 2, +1); // far
    uniformCullingBufferObject.planes[5] = extractPlane(viewProjection, 2, -1); // near

    memcpy(uniformCullingBuffersMapped[currentFrame], &uniformCullingBufferObject, sizeof(uniformCullingBufferObject));
}


// Matrices
void BufferManager::updateModelDataBuffer(uint32_t currentFrame) {
    static size_t frame = 0;

    if (modelEntityManager->dirty[0]) {
        auto* dst = static_cast<glm::vec4*>(modelDataBuffersMapped[currentFrame]);
        size_t i = 0;
        for (const auto& group : std::views::transform(modelEntityManager->groups, &ModelGroup::instances)) {
            for (const auto& model_instance : group) {
                dst[i * 3 + 0] = model_instance.pos;
                dst[i * 3 + 1] = model_instance.rot;
                dst[i * 3 + 2] = model_instance.scl;
                i++;
            }
        }

        frame++;
        if (frame == MAX_FRAMES_IN_FLIGHT) modelEntityManager->dirty[0] = false;
    }
}

void BufferManager::initializeModelBuffer() {
    if (!modelBufferInitialized) {
        matBufferObject.models.clear();
        matBufferObject.models.resize(modelEntityManager->getTotalInstanceCount());
        for (const auto& object : modelBuffersMapped) {
            memcpy(object, matBufferObject.models.data(), sizeof(glm::mat4) * matBufferObject.models.size());
        }

        modelBufferInitialized = true;
    }
}

void BufferManager::updateModelBuffer() {
    if (modelEntityManager->dirty[1]) {
        matBufferObject.models.resize(modelEntityManager->getTotalInstanceCount());

        modelEntityManager->dirty[1] = false;
    }
}

void BufferManager::updateSingleModel(const std::string &name, const uint32_t instanceIndex, glm::mat4 matrix) {
    if (!modelBufferInitialized) return;

    const auto scale = modelEntityManager->groups[modelEntityManager->indices[name]].instances[instanceIndex].scl;
    matrix[0] = glm::normalize(matrix[0]) * scale.x;
    matrix[1] = glm::normalize(matrix[1]) * scale.y;
    matrix[2] = glm::normalize(matrix[2]) * scale.z;
    matBufferObject.models[instanceIndex] = matrix;

    size_t modelIndex = 0;
    for (int i = 0; i < modelEntityManager->indexedFiles.size(); ++i) {
        if (modelEntityManager->indexedFiles[i] == name) modelIndex = modelEntityManager->indexedFiles.size() - 1;
    }

    const size_t globalInstanceIdx = modelIndex + instanceIndex;
    const size_t matOffset = globalInstanceIdx * sizeof(glm::mat4);
    const size_t sphereOffset = globalInstanceIdx * sizeof(CullingData);

    for (void* mappedPtr : modelBuffersMapped) {
        void* targetAddress = static_cast<char*>(mappedPtr) + matOffset;

        memcpy(targetAddress, &matrix, sizeof(glm::mat4));
    }

    matCullingBufferObject.cullingDatas[globalInstanceIdx].sphere = modelEntityManager->groups[modelEntityManager->indices[name]].model->sphere + glm::vec4(matrix[3]);
    void* targetAddress = static_cast<char*>(modelCullingBufferMapped) + sphereOffset;
    memcpy(targetAddress, &matCullingBufferObject.cullingDatas[globalInstanceIdx], sizeof(CullingData));
}

void BufferManager::addModel(const std::string &name, glm::vec4 pos) {
    if (!modelBufferInitialized) return;

    modelEntityManager->staticInstance(name, pos);

    glm::mat4 identityMatrix = glm::mat4(1.0f);
    glm::mat4 matrix = glm::translate(identityMatrix, glm::vec3(pos.x, pos.y, pos.z));

    matBufferObject.models.emplace_back(matrix);


    const size_t totalModels = modelEntityManager->getTotalInstanceCount();
    const size_t matOffset = (totalModels - 1) * sizeof(glm::mat4);
    const size_t sphereOffset = (totalModels - 1) * sizeof(CullingData);

    // Model buffer
    for (void* mappedPtr : modelBuffersMapped) {
        void* targetAddress = static_cast<char*>(mappedPtr) + matOffset;

        memcpy(targetAddress, &matrix, sizeof(glm::mat4));
    }

    // Culling buffer
    /*
    size_t modelIndex = 0;
    for (int i = 0; i < modelEntityManager->indexedFiles.size(); ++i) {
        if (modelEntityManager->indexedFiles[i] == name) modelIndex = modelEntityManager->indexedFiles.size() - 1;
    }
    matCullingBufferObject.cullingDatas.emplace_back(
        modelEntityManager->groups[modelEntityManager->indices[name]].model->sphere + glm::vec4(matrix[3]),
        modelIndex
        );
    void* targetAddress = static_cast<char*>(modelCullingBufferMapped) + sphereOffset;
    memcpy(targetAddress, &matCullingBufferObject.cullingDatas.back(), sizeof(CullingData));
    */

    auto* commands = static_cast<VkDrawIndexedIndirectCommand*>(drawCommandsSourceBuffersMapped[0]);

    uint32_t modelIdx = 1;
    uint32_t gpuSideInstanceCount = commands[modelIdx].instanceCount;

    std::cout << "GPU updated instance count: " << gpuSideInstanceCount << std::endl;
}


// Culling
void BufferManager::updateModelCullingBuffer() {
    if (modelEntityManager->dirty[2]) {
        matCullingBufferObject.cullingDatas.clear();
        matCullingBufferObject.cullingDatas.reserve(modelEntityManager->getTotalInstanceCount());

        uint16_t index = 0;
        for (const auto& group : std::views::transform(modelEntityManager->groups, &ModelGroup::instances)) {
            for (const auto& model_instance : group) {
                glm::vec4 sphere = model_instance.mdl.lock()->sphere + model_instance.pos;
                sphere.w *= glm::compMax(model_instance.scl);
                matCullingBufferObject.cullingDatas.emplace_back(sphere, index);
            }

            index++;
        }

        memcpy(modelCullingBufferMapped, matCullingBufferObject.cullingDatas.data(), matCullingBufferObject.cullingDatas.size() * sizeof(CullingData));

        modelEntityManager->dirty[2] = false;
    }
}

void BufferManager::updateVisibleIndicesBuffer() {
    static bool initialized = false;

    if (!initialized) {
        visibleIndicesObject.vi.clear();
        for (const auto& object : visibleIndicesBuffersMapped) {
            memcpy(object, visibleIndicesObject.vi.data(), visibleIndicesObject.vi.size() * sizeof(uint32_t));
        }
    }
}

void BufferManager::updateDrawCommands() {
    drawCommandsSourceObject.commands.clear();

    std::vector<uint32_t> firstIndex{0};
    std::vector<uint32_t> vertexOffset{0};

    for (const auto& group : modelEntityManager->groups) {
        const uint32_t indexCount = ModelEntityManager::getIndexCount(group.model);

        firstIndex.emplace_back(firstIndex.back() + indexCount);
        vertexOffset.emplace_back(vertexOffset.back() + ModelEntityManager::getVertexCount(group.model));
    }

    drawCommandsSourceObject.commands.clear();

    uint32_t firstInstance = 0;
    for (int i = 0; i < modelEntityManager->regions.size(); i++) {
        VkDrawIndexedIndirectCommand command{};

        const auto& region = modelEntityManager->regions[i];
        auto& group = modelEntityManager->groups[region.modelIndex];

        const size_t instanceCount = region.count;
        const uint32_t indexCount = ModelEntityManager::getIndexCount(group.model);

        command.firstInstance = firstInstance;
        command.instanceCount = instanceCount;

        command.firstIndex = firstIndex[region.modelIndex];
        command.indexCount = indexCount;
        command.vertexOffset = vertexOffset[region.modelIndex];

        firstInstance += instanceCount;

        drawCommandsSourceObject.commands.emplace_back(command);
    }

    for (auto& command : drawCommandsSourceObject.commands) {
        std::cout << "instanceCount: " << command.instanceCount << std::endl;
        std::cout << "indexCount: " << command.indexCount << std::endl;
        std::cout << "firstIndex: " << command.firstIndex << std::endl;
        std::cout << "firstInstance: " << command.firstInstance << std::endl;
        std::cout << "vertexOffset: " << command.vertexOffset << std::endl;
    }
}

void BufferManager::updateTextureIndexBuffer() {
    static bool initialized = false;

    if (!initialized) {
        size_t i = 0;
        const size_t count = modelEntityManager->getTotalModelCount();
        uint32_t currentGlobalVertexOffset = 0;

        textureIndexBufferObject.indices.resize(count);

        for (const auto& model : modelEntityManager->groups | std::views::transform(&ModelGroup::model)) {
            auto& textures = model->textures;

            if (textures.empty()) {
                textureIndexBufferObject.indices[i].firstIndex = 0;
                textureIndexBufferObject.indices[i].indexCount = 0;
            } else {
                textureIndexBufferObject.indices[i].firstIndex = textures[0].index;
                textureIndexBufferObject.indices[i].indexCount = textures.size();
                textureIndexBufferObject.indices[i].globalVertexOffset = currentGlobalVertexOffset;

                //std::cout << "==================" << std::endl;
                //std::cout << "First index: " << textures[0].index << std::endl;
                //std::cout << "Tex count: " << textureIndexBuffer.indices[i].indexCount << std::endl;
                //std::cout << "Padding index: " << textureIndexBuffer.indices[i].globalVertexOffset << std::endl;
                //std::cout << "==================" << std::endl;
                for (const auto & texture : textures) {
                    const uint32_t offset = texture.indexOffset;
                    TextureIndexOffsetBufferObject.indexOffsets.emplace_back(offset);
                    //std::cout << "Tex offset: " << offset << std::endl;
                }
            }

            uint32_t modelTotalVertices = 0;
            for (const auto& mesh : model->meshes) {
                modelTotalVertices += static_cast<uint32_t>(mesh.size());
            }
            currentGlobalVertexOffset += modelTotalVertices;

            i++;
        }

        //std::cout << "Index offsets count: " << TextureIndexOffsetBuffer.indexOffsets.size() << std::endl;
        //std::cout << "Index offsets2 count: " << textureIndexBuffer.indices.size() << std::endl;

        memcpy(textureIndexBufferMapped, textureIndexBufferObject.indices.data(), sizeof(Index) * textureIndexBufferObject.indices.size());
        memcpy(textureIndexOffsetBufferMapped, TextureIndexOffsetBufferObject.indexOffsets.data(), TextureIndexOffsetBufferObject.indexOffsets.size() * sizeof(uint32_t));

        initialized = true;
    }
}
#pragma endregion

void BufferManager::createBuffers(VkQueue graphicsQueue, VkCommandPool graphicsCommandPool) {
    BuffersRegistry::createVertexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, *modelEntityManager);
    BuffersRegistry::createIndexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, indexBuffer, indexBufferMemory, *modelEntityManager);


    // Generic
    BuffersRegistry::createGenericBuffers(device, physicalDevice, uniformConstants, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, uniformMatrixConstants, uniformMatrixBuffers, uniformMatrixBuffersMemory, uniformMatrixBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, uniformPostprocessingConstants, uniformPostprocessingBuffers, uniformPostprocessingBuffersMemory, uniformPostprocessingBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(uniformPostprocessingBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, atomicCounterConstants, atomicCounterBuffers, atomicCounterBuffersMemory, atomicCounterBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


    // Matrices
    BuffersRegistry::createGenericBuffers(device, physicalDevice, modelConstants, modelBuffers, modelBuffersMemory, modelBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(glm::mat4) * 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, modelDataConstants, modelDataBuffers, modelDataBuffersMemory, modelDataBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(glm::vec4) * 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Culling
    BuffersRegistry::createGenericBuffers(device, physicalDevice, uniformCullingConstants, uniformCullingBuffers, uniformCullingBuffersMemory, uniformCullingBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformCullingBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, visibleIndicesConstants, visibleIndicesBuffers, visibleIndicesBuffersMemory, visibleIndicesBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(uint32_t) * 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffer(device, physicalDevice, modelCullingConstant, modelCullingBuffer, modelCullingBufferMemory, modelCullingBufferMapped, sizeof(CullingData) * 2048, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


    BuffersRegistry::createGenericBuffer(device, physicalDevice, textureIndexConstant, textureIndexBuffer, textureIndexBufferMemory, textureIndexBufferMapped, sizeof(uint32_t) * 4 * 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    BuffersRegistry::createGenericBuffer(device, physicalDevice, textureIndexOffsetConstant, textureIndexOffsetBuffer, textureIndexOffsetBufferMemory, textureIndexOffsetBufferMapped, sizeof(uint32_t) * 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // Draw commands shenanigans
    updateDrawCommands();
    BuffersRegistry::createGenericBuffers(device, physicalDevice, drawCommandsSourceConstants, drawCommandsSourceBuffers, drawCommandsSourceBuffersMemory, drawCommandsSourceBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(VkDrawIndexedIndirectCommand) * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    BuffersRegistry::createGenericBuffers(device, physicalDevice, drawCommandsConstants, drawCommandsBuffers, drawCommandsBuffersMemory, drawCommandsBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(VkDrawIndexedIndirectCommand) * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        memcpy(drawCommandsSourceBuffersMapped[i], drawCommandsSourceObject.commands.data(), sizeof(VkDrawIndexedIndirectCommand) * drawCommandsSourceObject.commands.size());
        //memcpy(drawCommandsBuffersMapped[i], drawCommandsObj.commands.data(), sizeof(VkDrawIndexedIndirectCommand) * drawCommandsObj.commands.size());
    }

    initializeModelBuffer();
}

void BufferManager::cleanup() const {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, uniformMatrixBuffers[i], nullptr);
        vkFreeMemory(device, uniformMatrixBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, modelBuffers[i], nullptr);
        vkFreeMemory(device, modelBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, modelDataBuffers[i], nullptr);
        vkFreeMemory(device, modelDataBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, atomicCounterBuffers[i], nullptr);
        vkFreeMemory(device, atomicCounterBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, visibleIndicesBuffers[i], nullptr);
        vkFreeMemory(device, visibleIndicesBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, uniformCullingBuffers[i], nullptr);
        vkFreeMemory(device, uniformCullingBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, uniformPostprocessingBuffers[i], nullptr);
        vkFreeMemory(device, uniformPostprocessingBuffersMemory[i], nullptr);
    }

    vkDestroyBuffer(device, modelCullingBuffer, nullptr);
    vkFreeMemory(device, modelCullingBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, textureIndexBuffer, nullptr);
    vkFreeMemory(device, textureIndexBufferMemory, nullptr);

    vkDestroyBuffer(device, textureIndexOffsetBuffer, nullptr);
    vkFreeMemory(device, textureIndexOffsetBufferMemory, nullptr);
}
