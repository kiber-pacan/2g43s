//
// Created by down1 on 29.12.2025.
//

#include "Descriptor.hpp"

#include <ranges>

#include "../../buffers/main/uniform/UniformBufferObject.hpp"
#include "UniformCullingBufferObject.hpp"

#pragma region Graphics
void Descriptor::createGraphicsDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& graphicsDescriptorSetLayout) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 256;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding mboLayoutBinding{};
    mboLayoutBinding.binding = 2;
    mboLayoutBinding.descriptorCount = 1;
    mboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mboLayoutBinding.pImmutableSamplers = nullptr;
    mboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding viboLayoutBinding{};
    viboLayoutBinding.binding = 3;
    viboLayoutBinding.descriptorCount = 1;
    viboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    viboLayoutBinding.pImmutableSamplers = nullptr;
    viboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding tioLayoutBinding{};
    tioLayoutBinding.binding = 4;
    tioLayoutBinding.descriptorCount = 1;
    tioLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tioLayoutBinding.pImmutableSamplers = nullptr;
    tioLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding tiooLayoutBinding{};
    tiooLayoutBinding.binding = 5;
    tiooLayoutBinding.descriptorCount = 1;
    tiooLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tiooLayoutBinding.pImmutableSamplers = nullptr;
    tiooLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    const std::array bindings = {uboLayoutBinding, samplerLayoutBinding, mboLayoutBinding, viboLayoutBinding, tioLayoutBinding, tiooLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &graphicsDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Descriptor::createGraphicsDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& graphicsDescriptorPool) {
    std::array<VkDescriptorPoolSize, 6> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 256;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[5].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &graphicsDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Descriptor::createGraphicsDescriptorSets(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout& graphicsDescriptorSetLayout, const VkDescriptorPool& graphicsDescriptorPool, std::vector<VkDescriptorSet>& graphicsDescriptorSets, const std::vector<VkBuffer>& uniformBuffers, const VkSampler& textureSampler, const std::vector<VkBuffer>& modelBuffers, const std::vector<VkBuffer>& visibleIndicesBuffers, const ModelBus& mdlBus, const VkImageView& textureImageView, const VkBuffer& textureIndexBuffer, const VkBuffer& textureIndexOffsetBuffer) {
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = graphicsDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, graphicsDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo uboBufferInfo{};
        uboBufferInfo.buffer = uniformBuffers[i];
        uboBufferInfo.offset = 0;
        uboBufferInfo.range = sizeof(UniformBufferObject);

        std::vector<VkDescriptorImageInfo> imageInfo{};
        VkDescriptorImageInfo missingno{};
        missingno.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        missingno.imageView = textureImageView;
        missingno.sampler = textureSampler;

        imageInfo.emplace_back(missingno);

        size_t index = 0;
        for (auto& textures : mdlBus.groups_map
        | std::views::values
        | std::views::transform(&ModelGroup::model)
        | std::views::transform(&ParsedModel::textures)
        ) {
            if (textures.empty()) continue;
            for (const auto texture : textures) {
                VkDescriptorImageInfo temp{};
                temp.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                temp.imageView = texture.textureImageView;
                temp.sampler = textureSampler;

                imageInfo.emplace_back(temp);

                index++;
            }
        }

        VkDescriptorBufferInfo mboBufferInfo{};
        mboBufferInfo.buffer = modelBuffers[i];
        mboBufferInfo.offset = 0;
        mboBufferInfo.range = mdlBus.getModelBufferSize();

        VkDescriptorBufferInfo viboBufferInfo{};
        viboBufferInfo.buffer = visibleIndicesBuffers[i];
        viboBufferInfo.offset = 0;
        viboBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo tioBufferInfo{};
        tioBufferInfo.buffer = textureIndexBuffer;
        tioBufferInfo.offset = 0;
        tioBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo tiooBufferInfo{};
        tiooBufferInfo.buffer = textureIndexOffsetBuffer;
        tiooBufferInfo.offset = 0;
        tiooBufferInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uboBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = imageInfo.size();
        descriptorWrites[1].pImageInfo = imageInfo.data();

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &mboBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &viboBufferInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &tioBufferInfo;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo = &tiooBufferInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
#pragma endregion


#pragma region Matrix
void Descriptor::createMatrixComputeDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& matrixComputeDescriptorSetLayout) {
    VkDescriptorSetLayoutBinding mdboLayoutBinding{};
    mdboLayoutBinding.binding = 0;
    mdboLayoutBinding.descriptorCount = 1;
    mdboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mdboLayoutBinding.pImmutableSamplers = nullptr;
    mdboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding mboLayoutBinding{};
    mboLayoutBinding.binding = 1;
    mboLayoutBinding.descriptorCount = 1;
    mboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mboLayoutBinding.pImmutableSamplers = nullptr;
    mboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::array bindings = {mdboLayoutBinding, mboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &matrixComputeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Descriptor::createMatrixComputeDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& matrixComputeDescriptorPool) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &matrixComputeDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Descriptor::createMatrixComputeDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout &matrixComputeDescriptorSetLayout, const VkDescriptorPool &matrixComputeDescriptorPool, std::vector<VkDescriptorSet> &matrixComputeDescriptorSets, const std::vector<VkBuffer> &modelBuffers, const ModelBus &mdlBus, const std::vector<VkBuffer> &modelDataBuffers) {
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, matrixComputeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = matrixComputeDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    matrixComputeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, matrixComputeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mdboBufferInfo{};
        mdboBufferInfo.buffer = modelDataBuffers[i];
        mdboBufferInfo.offset = 0;
        mdboBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo mboBufferInfo{};
        mboBufferInfo.buffer = modelBuffers[i];
        mboBufferInfo.offset = 0;
        mboBufferInfo.range = mdlBus.getModelBufferSize();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = matrixComputeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mdboBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = matrixComputeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &mboBufferInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
#pragma endregion


#pragma region Culling
void Descriptor::createCullingComputeDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& cullingComputeDescriptorSetLayout) {
    VkDescriptorSetLayoutBinding mbcLayoutBinding{};
    mbcLayoutBinding.binding = 0;
    mbcLayoutBinding.descriptorCount = 1;
    mbcLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    mbcLayoutBinding.pImmutableSamplers = nullptr;
    mbcLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding viLayoutBinding{};
    viLayoutBinding.binding = 1;
    viLayoutBinding.descriptorCount = 1;
    viLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    viLayoutBinding.pImmutableSamplers = nullptr;
    viLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding drawCommandsLayoutBinding{};
    drawCommandsLayoutBinding.binding = 2;
    drawCommandsLayoutBinding.descriptorCount = 1;
    drawCommandsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    drawCommandsLayoutBinding.pImmutableSamplers = nullptr;
    drawCommandsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding ucboLayoutBinding{};
    ucboLayoutBinding.binding = 3;
    ucboLayoutBinding.descriptorCount = 1;
    ucboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ucboLayoutBinding.pImmutableSamplers = nullptr;
    ucboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding atomicCounterBinding{};
    atomicCounterBinding.binding = 4;
    atomicCounterBinding.descriptorCount = 1;
    atomicCounterBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    atomicCounterBinding.pImmutableSamplers = nullptr;
    atomicCounterBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::array bindings = {mbcLayoutBinding, viLayoutBinding, drawCommandsLayoutBinding, ucboLayoutBinding, atomicCounterBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &cullingComputeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Descriptor::createCullingComputeDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& cullingComputeDescriptorPool) {
    std::array<VkDescriptorPoolSize, 5> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &cullingComputeDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Descriptor::createCullingComputeDescriptorSets(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout& cullingComputeDescriptorSetLayout,
                                                    const VkDescriptorPool& cullingComputeDescriptorPool, std::vector<VkDescriptorSet>& cullingComputeDescriptorSets,
                                                    const ModelBus& mdlBus, const VkBuffer&  drawCommandsBuffer, const std::vector<VkBuffer>& uniformCullingBuffers,
                                                    const std::vector<VkBuffer>& visibleIndicesBuffers, const VkBuffer& modelCullingBuffer,
                                                    const std::vector<VkBuffer>& atomicCounterBuffers) {
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, cullingComputeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = cullingComputeDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    cullingComputeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, cullingComputeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo mcbInfo{};
        mcbInfo.buffer = modelCullingBuffer;
        mcbInfo.offset = 0;
        mcbInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo vibInfo{};
        vibInfo.buffer = visibleIndicesBuffers[i];
        vibInfo.offset = 0;
        vibInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo drawCommandsInfo{};
        drawCommandsInfo.buffer = drawCommandsBuffer;
        drawCommandsInfo.offset = 0;
        drawCommandsInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo ucbInfo{};
        ucbInfo.buffer = uniformCullingBuffers[i];
        ucbInfo.offset = 0;
        ucbInfo.range = sizeof(UniformCullingBufferObject);

        VkDescriptorBufferInfo atomicCounterInfo{};
        atomicCounterInfo.buffer = atomicCounterBuffers[i];
        atomicCounterInfo.offset = 0;
        atomicCounterInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = cullingComputeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &mcbInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = cullingComputeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &vibInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = cullingComputeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &drawCommandsInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = cullingComputeDescriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &ucbInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = cullingComputeDescriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &atomicCounterInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
#pragma endregion


#pragma region Postprocess
void Descriptor::createPostprocessDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayout& postProcessingDescriptorSetLayout) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding depthSamplerLayoutBinding{};
    depthSamplerLayoutBinding.binding = 1;
    depthSamplerLayoutBinding.descriptorCount = 1;
    depthSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthSamplerLayoutBinding.pImmutableSamplers = nullptr;
    depthSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 2;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    constexpr VkDescriptorBindingFlagsEXT flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;

    const std::array bindings = {samplerLayoutBinding, uboLayoutBinding, depthSamplerLayoutBinding};

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
    binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    binding_flags.bindingCount = static_cast<uint32_t>(bindings.size());
    binding_flags.pBindingFlags = &flags;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.pNext = &binding_flags;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &postProcessingDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Descriptor::createPostprocessDescriptorPool(const VkDevice& device, const int& MAX_FRAMES_IN_FLIGHT, VkDescriptorPool& postProcessingDescriptorPool) {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &postProcessingDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Descriptor::createPostprocessDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const VkDescriptorSetLayout &postProcessingDescriptorSetLayout, const
                                                 VkDescriptorPool &postProcessingDescriptorPool, std::vector<VkDescriptorSet> &postProcessingDescriptorSets, const std::
                                                 vector<VkImageView> &offscreenImageViews, const VkImageView& depthImageView, const VkSampler &textureSampler, const std::vector<VkBuffer> &
                                                 uniformPostprocessingBuffers) {
    const std::vector layouts(MAX_FRAMES_IN_FLIGHT, postProcessingDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = postProcessingDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    postProcessingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, postProcessingDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = offscreenImageViews[i];
        imageInfo.sampler = textureSampler;

        VkDescriptorImageInfo depthImageInfo{};
        depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthImageInfo.imageView = depthImageView;
        depthImageInfo.sampler = textureSampler;

        VkDescriptorBufferInfo upboBufferInfo{};
        upboBufferInfo.buffer = uniformPostprocessingBuffers[i];
        upboBufferInfo.offset = 0;
        upboBufferInfo.range = sizeof(UniformBufferObject);

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = postProcessingDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = postProcessingDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &depthImageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = postProcessingDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &upboBufferInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

void Descriptor::updatePostprocessDescriptorSets(const VkDevice &device, const int &MAX_FRAMES_IN_FLIGHT, const std::vector<VkDescriptorSet> &postProcessingDescriptorSets, const std::vector<VkImageView> &offscreenImageViews, const VkImageView &imageViews, const VkSampler &textureSampler) {
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    VkDescriptorImageInfo imageInfos{};
    VkDescriptorImageInfo depthImageInfos{};

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.imageView = offscreenImageViews[i];
        imageInfos.sampler = textureSampler;

        depthImageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthImageInfos.imageView = imageViews;
        depthImageInfos.sampler = textureSampler;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].dstSet = postProcessingDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].pImageInfo = &imageInfos;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].dstSet = postProcessingDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].pImageInfo = &depthImageInfos;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}
#pragma endregion