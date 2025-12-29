//
// Created by down1 on 29.12.2025.
//

#include "Command.hpp"

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
        VkPipelineLayout& graphicsPipelineLayout,
        VkPipelineLayout& matrixComputePipelineLayout,
        VkPipelineLayout& cullingComputePipelineLayout,
        std::vector<VkDescriptorSet>& graphicsDescriptorSets,
        std::vector<VkDescriptorSet>& matrixComputeDescriptorSets,
        std::vector<VkDescriptorSet>& cullingComputeDescriptorSets,
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
        std::vector<VkImage>& swapchainImages
    ) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        vkCmdFillBuffer(commandBuffer, drawCommandsBuffer, offsetof(VkDrawIndexedIndirectCommand, instanceCount), 4, 0); // Clear 4 bites with offset of 4

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

            VkBufferMemoryBarrier mdbBarrier{};
            mdbBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            mdbBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            mdbBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            mdbBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            mdbBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            mdbBarrier.buffer = modelDataBuffers[currentFrame];
            mdbBarrier.offset = 0;
            mdbBarrier.size = VK_WHOLE_SIZE;

            vkCmdDispatch(commandBuffer, groupCount, 1, 1);
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,  nullptr, 1, &mdbBarrier, 0, nullptr);
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

            VkBufferMemoryBarrier acbBarrier{};
            acbBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            acbBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            acbBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            acbBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            acbBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            acbBarrier.buffer = atomicCounterBuffers[currentFrame];
            acbBarrier.offset = 0;
            acbBarrier.size = VK_WHOLE_SIZE;

            VkBufferMemoryBarrier vibBarrier{};
            vibBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            vibBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            vibBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vibBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vibBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vibBarrier.buffer = visibleIndicesBuffers[currentFrame];
            vibBarrier.offset = 0;
            vibBarrier.size = VK_WHOLE_SIZE;

            std::array<VkBufferMemoryBarrier, 2> barriers = { acbBarrier, vibBarrier };

            vkCmdDispatch(commandBuffer, groupCount, 1, 1);
            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                0,
                0, nullptr,
                static_cast<uint32_t>(barriers.size()), barriers.data(),
                0, nullptr
            );
        }
        frame1++;
        #pragma endregion

        // Dynamic rendering info
        #pragma region renderInfo
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchainImageViews[imageIndex];
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

        // Syncing image layouts because of new dynamic rendering
        #pragma region imageSync
        VkImageMemoryBarrier depthBarrier{};
        depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthBarrier.srcAccessMask = 0;
        depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.image = depthImage;
        depthBarrier.subresourceRange.aspectMask =  VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        depthBarrier.subresourceRange.baseMipLevel = 0;
        depthBarrier.subresourceRange.levelCount = 1;
        depthBarrier.subresourceRange.baseArrayLayer = 0;
        depthBarrier.subresourceRange.layerCount = 1;

        VkImageMemoryBarrier colorBarrier{};
        colorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        colorBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorBarrier.srcAccessMask = 0;
        colorBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        colorBarrier.image = swapchainImages[imageIndex];
        colorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorBarrier.subresourceRange.baseMipLevel = 0;
        colorBarrier.subresourceRange.levelCount = 1;
        colorBarrier.subresourceRange.baseArrayLayer = 0;
        colorBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,0,0, nullptr,0, nullptr,1, &depthBarrier);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);
        #pragma endregion

        #pragma region render
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

            vkCmdDrawIndexedIndirect(commandBuffer, drawCommandsBuffer, 0, mdlBus.getTotalModelCount(), sizeof(VkDrawIndexedIndirectCommand));

            drawImGui(commandBuffer);
        vkCmdEndRendering(commandBuffer);
        #pragma endregion

        #pragma region layoutChange
        colorBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        colorBarrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);
        #pragma endregion

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }