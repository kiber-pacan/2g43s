//
// Created by down1 on 29.12.2025.
//

#include "Command.hpp"

#include "Barrier.h"

void Command::createCommandPool(const VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkSurfaceKHR& surface) {
    auto [graphicsFamily, presentFamily] = Queue::findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

VkCommandBuffer Command::beginSingleTimeCommands(const VkDevice& device, const VkCommandPool& commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Command::endSingleTimeCommands(const VkDevice& device, const VkCommandBuffer& commandBuffer, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Command::recordCommandBuffer(
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
    ) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdFillBuffer(commandBuffer, atomicCounterBuffers[currentFrame], 0, sizeof(uint32_t), 0); // Clear atomic counter
    for (int i = 0; i < mdlBus.getTotalModelCount(); i++) {
        vkCmdFillBuffer(commandBuffer, drawCommandsBuffer, offsetof(VkDrawIndexedIndirectCommand, instanceCount) + sizeof(VkDrawIndexedIndirectCommand) * i, 4, 0); // Clear 4 bites with offset of 4
    }



    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {
        {
            static_cast<float>(clear_color.r) / 255,
            static_cast<float>(clear_color.g) / 255,
            static_cast<float>(clear_color.b) / 255,
            static_cast<float>(clear_color.a) / 255
        }
    };
    clearValues[1].depthStencil = {1.0f, 0};

    #pragma region Matrices
    static size_t frame = 0;

    if (matrixDirty) {
        constexpr uint32_t workgroupSize = 128;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, matrixComputePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, matrixComputePipelineLayout, 0, 1, &matrixComputeDescriptorSets[currentFrame], 0, nullptr);

        frame++;
        if (frame == MAX_FRAMES_IN_FLIGHT) {
            matrixDirty = false;
            frame = 0;
        }

        const size_t totalInstances = mdlBus.getTotalInstanceCount();
        uint32_t groupCount = (totalInstances + workgroupSize - 1) / workgroupSize;

        vkCmdDispatch(commandBuffer, groupCount, 1, 1);
        Barrier vibBarrier(commandBuffer);
        vibBarrier.buffer(
            modelDataBuffers[currentFrame],
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            0, VK_WHOLE_SIZE
            ).apply();
    }
    #pragma endregion

    #pragma region Culling
    static size_t frame1 = 0;

    if (frame1 % 4 == 0 || true) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cullingComputePipelineLayout, 0, 1, &cullingComputeDescriptorSets[currentFrame], 0, nullptr);

        constexpr uint32_t workgroupSize1 = 128;
        const size_t totalInstances = mdlBus.getTotalInstanceCount();
        uint32_t groupCount = (totalInstances + workgroupSize1 - 1) / workgroupSize1;

        vkCmdDispatch(commandBuffer, groupCount, 1, 1);
        Barrier vibBarrier(commandBuffer);
        vibBarrier.buffer(
        atomicCounterBuffers[currentFrame],
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        0, VK_WHOLE_SIZE
        ).buffer(
        visibleIndicesBuffers[currentFrame],
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        0, VK_WHOLE_SIZE
        ).apply();
    }
    frame1++;
    #pragma endregion

    // Dynamic rendering info
    #pragma region offScreenRenderInfo
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = offscreenImageViews[currentFrame];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValues[0];

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, swapchainExtent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    #pragma endregion

    // Transition image layouts so we can write in it
    #pragma region initialImageLayoutTransition
    Barrier barrier(commandBuffer);

    barrier.image(
    depthImage,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ACCESS_2_NONE, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
    ).image(
    offscreenImages[currentFrame],
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    VK_ACCESS_2_NONE, VK_ACCESS_2_NONE,
    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
    ).apply();
    #pragma endregion

    // Render scene without screen elements
    #pragma region offScreenRender
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &graphicsDescriptorSets[currentFrame], 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    //vkCmdDrawIndexedIndirectCount(commandBuffer, drawCommandsBuffer, 0, atomicCounterBuffers[currentFrame], 0, 256, sizeof(VkDrawIndexedIndirectCommand));

    vkCmdDrawIndexedIndirect(commandBuffer, drawCommandsBuffer, 0, mdlBus.getTotalModelCount(), sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRendering(commandBuffer);
    #pragma endregion

    // Transition Offscreen image layout so we can read it from shaders
    #pragma region offscreenLayoutTransition
    Barrier midBarrier(commandBuffer);
    barrier.image(
    offscreenImages[currentFrame],
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
    ).image(
    depthImage,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
    ).image(
    swapchainImages[imageIndex],
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
    ).apply();
    #pragma endregion

    #pragma region screenRenderInfo
    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = swapchainImageViews[imageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfoScreen{};
    renderingInfoScreen.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfoScreen.renderArea = {{0, 0}, swapchainExtent};
    renderingInfoScreen.layerCount = 1;
    renderingInfoScreen.colorAttachmentCount = 1;
    renderingInfoScreen.pColorAttachments = &colorAttachmentInfo;
    #pragma endregion

    // Render face with offscreen image so we can do postprocessing
    #pragma region renderScreen
    vkCmdBeginRendering(commandBuffer, &renderingInfoScreen);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocessPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocessPipelineLayout, 0, 1, &postprocessDescriptorSets[currentFrame], 0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    drawImGui(commandBuffer);
    vkCmdEndRendering(commandBuffer);
    #pragma endregion

    // Transition swapchain layout so we can show it on screen
    #pragma region swapchainLayoutTransition
    Barrier swapchainBarrier(commandBuffer);

    swapchainBarrier.image(
    swapchainImages[imageIndex],
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_NONE,
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT
    ).apply();
    #pragma endregion

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}