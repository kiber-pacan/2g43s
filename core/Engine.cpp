#ifndef ENGINE_CPP
#define ENGINE_CPP
#include <vulkan/vulkan_core.h>
#include "Engine.hpp"

#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include "Random.hpp"
#include "MatrixPushConstants.hpp"
#include "PostprocessPushConstants.hpp"
#include "VertexPushConstants.hpp"
#include "CullingPushConstants.hpp"

#include "Descriptor.hpp"
#include "Tools.hpp"
#include "Surface.hpp"
#include "Color.hpp"
#include "Barrier.h"
#include "Helper.hpp"
#include "Images.hpp"
#include "PipelineCreation.hpp"
#include "Command.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "Buffers.hpp"
#include "sep/model/managing/PhysicsBus.h"
#include "Shaders.hpp"
#include "Sync.hpp"

#pragma region Main
// Method for initializing and running engine
void Engine::init(SDL_Window* sdl_window) {
    initialize(sdl_window);
}

void Engine::drawImGui(const VkCommandBuffer& commandBuffer) {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();

    ImGui::NewFrame();

    //ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), viewport->Size);

    ImGui::Begin("Stats");

    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();

    const float minX = viewport->Pos.x;
    const float minY = viewport->Pos.y;
    const float maxX = std::max(minX, viewport->Pos.x + viewport->Size.x - size.x);
    const float maxY = std::max(minY, viewport->Pos.y + viewport->Size.y - size.y);

    const float clampedX = std::clamp(pos.x, minX, maxX);
    const float clampedY = std::clamp(pos.y, minY, maxY);

    if (pos.x != clampedX || pos.y != clampedY) {
        ImGui::SetWindowPos(ImVec2(clampedX, clampedY));
    }

    ImGui::SetNextWindowSize(ImVec2(200,160), ImGuiCond_Appearing);

    // FPS
    ImGui::Value("Framerate", static_cast<int>(delta.frameRate));
    ImGui::Value("Total models", static_cast<int>(mem.modelsCount()));
    ImGui::Value("Total instances", static_cast<int>(mem.getTotalInstanceCount()));

    int desiredFps = static_cast<int>(desiredFrameRate);

    if (ImGui::BeginTable("fps_table", 2)) {
        ImGui::TableNextRow();

        const auto label = "Desired FPS";
        const float textWidth = ImGui::CalcTextSize(label).x;
        const float fpsWidth = ImGui::CalcTextSize(std::to_string(desiredFrameRate).c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;

        ImGui::TableSetColumnIndex(0);
        ImGui::SetNextItemWidth(textWidth);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(label);

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(fpsWidth);
        if (ImGui::InputInt("##desiredFps", &desiredFps, 0)) {
            desiredFrameRate = desiredFps;
            sleepTimeTotalSeconds = 1.0 / std::clamp(desiredFrameRate, 5.0, std::numeric_limits<double>::max());
        }

        ImGui::EndTable();
    }

    const std::string selectedShader1 = selectedShader;
    if (ImGui::BeginCombo("shaders", selectedShader1.c_str())) {
        for (const std::string& shader : postprocessPipelines | std::views::keys) {

            const bool is_selected = shader == selectedShader;

            if (ImGui::Selectable(shader.c_str(), is_selected)) {
                selectedShader = shader;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Engine::recordCommandBuffer(uint32_t imageIndex, VkPipeline postprocessPipeline) {
    VkCommandBuffer& commandBuffer = commandBuffers[currentFrame];
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }


    #pragma region Cleanup
    vkCmdFillBuffer(commandBuffer, atomicCounterBuffers[currentFrame], 0, sizeof(uint32_t), 0); // Clear atomic counter
    for (int i = 0; i < mem.getTotalModelCount(); i++) {
        vkCmdFillBuffer(commandBuffer, drawCommandsSourceBuffers[currentFrame], offsetof(VkDrawIndexedIndirectCommand, instanceCount) + sizeof(VkDrawIndexedIndirectCommand) * i, 4, 0); // Clear 4 bites with offset of 4
    }
    #pragma endregion

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

        MatrixPushConstants matrixConstants{};
        matrixConstants.mdb = modelDataConstants[currentFrame];
        matrixConstants.mb = modelConstants[currentFrame];
        matrixConstants.umbo = uniformMatrixConstants[currentFrame];

        vkCmdPushConstants(
        commandBuffer,
        matrixComputePipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(MatrixPushConstants),
        &matrixConstants
        );

        frame++;
        if (frame == MAX_FRAMES_IN_FLIGHT) {
            matrixDirty = false;
            frame = 0;
        }

        const size_t totalInstances = mem.getTotalInstanceCount();
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

        CullingPushConstants cullingConstants{};
        cullingConstants.mcb = modelCullingConstant;
        cullingConstants.vib = visibleIndicesConstants[currentFrame];
        cullingConstants.dcsb = drawCommandsSourceConstants[currentFrame];
        cullingConstants.dcb = drawCommandsConstants[currentFrame];
        cullingConstants.ucbo = uniformCullingConstants[currentFrame];
        cullingConstants.counter = atomicCounterConstants[currentFrame];

        vkCmdPushConstants(
        commandBuffer,
        cullingComputePipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(CullingPushConstants),
        &cullingConstants
        );

        constexpr uint32_t workgroupSize1 = 128;
        const size_t totalInstances = mem.getTotalInstanceCount();
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
        ).buffer(
            drawCommandsSourceBuffers[currentFrame],
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
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

    VertexPushConstants vertexConstants{};
    vertexConstants.ubo = uniformConstants[currentFrame];
    vertexConstants.mb = modelConstants[currentFrame];
    vertexConstants.vib = visibleIndicesConstants[currentFrame];
    vertexConstants.ti = textureIndexConstant;
    vertexConstants.tio = textureIndexOffsetConstant;

    vkCmdPushConstants(
    commandBuffer,
    graphicsPipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT,
    0,
    sizeof(VertexPushConstants),
    &vertexConstants
    );

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

    vkCmdDrawIndexedIndirectCount(commandBuffer, drawCommandsSourceBuffers[currentFrame], 0, atomicCounterBuffers[currentFrame], 0, 1024, sizeof(VkDrawIndexedIndirectCommand));
    //vkCmdDrawIndexedIndirect(commandBuffer, drawCommandsSourceBuffers[currentFrame], 0, static_cast<uint32_t>(mdlBus.getTotalModelCount()), sizeof(VkDrawIndexedIndirectCommand));

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

    PostprocessPushConstants postprocessingConstants{};
    postprocessingConstants.mpbo = uniformPostprocessingConstants[currentFrame];

    vkCmdPushConstants(
    commandBuffer,
    postprocessPipelineLayout,
    VK_SHADER_STAGE_FRAGMENT_BIT,
    0,
    sizeof(PostprocessPushConstants),
    &postprocessingConstants
    );

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

void Engine::waitForFences(const VkFence& fence) const {
    Logger logger("waitForFences()");

    const auto result = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    if (result != VK_SUCCESS) {
        logger.error("ERROR!");
    } else {
        vkResetFences(device, 1, &fence);
    }
}

// Draw
void Engine::drawFrame() {
    waitForFences(imageFences[currentFrame]);
    waitForFences(inFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer();
    updateUniformPostprocessingBuffer();
    updateCullingUniformBuffer();
    updateModelCullingBuffer();
    updateModelDataBuffer();
    updateModelBuffer();
    updateTextureIndexBuffer();

    std::function<void(VkCommandBuffer&)> imGui = [this](const VkCommandBuffer& commandBuffer) { drawImGui(commandBuffer); };

    recordCommandBuffer(imageIndex, postprocessPipelines[selectedShader]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    const VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainPresentFenceInfoKHR imageFenceInfo{};
    imageFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR;
    imageFenceInfo.pFences = &imageFences[currentFrame];
    imageFenceInfo.swapchainCount = 1;

    const VkSwapchainKHR swapchains[] = { swapchain };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pNext = &imageFenceInfo;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


// Clean trash before closing app
void Engine::cleanup() const {
    vkDeviceWaitIdle(device);

    cleanupSwapchain();
    cleanupOffscreenImages();

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    for (auto& textures : mem.groups
    | std::views::values
    | std::views::transform(&ModelGroup::model)
    | std::views::transform(&ParsedModel::textures)
    ) {
        for (const auto& texture : textures) {
            vkDestroyImageView(device, texture.textureImageView, nullptr);
            vkDestroyImage(device, texture.textureImage, nullptr);
            vkFreeMemory(device, texture.textureImageMemory, nullptr);
        }
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformMatrixBuffers[i], nullptr);
        vkFreeMemory(device, uniformMatrixBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, modelBuffers[i], nullptr);
        vkFreeMemory(device, modelBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, modelDataBuffers[i], nullptr);
        vkFreeMemory(device, modelDataBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, atomicCounterBuffers[i], nullptr);
        vkFreeMemory(device, atomicCounterBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, visibleIndicesBuffers[i], nullptr);
        vkFreeMemory(device, visibleIndicesBuffersMemory[i], nullptr);
    }

    vkDestroyBuffer(device, modelCullingBuffer, nullptr);
    vkFreeMemory(device, modelCullingBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformCullingBuffers[i], nullptr);
        vkFreeMemory(device, uniformCullingBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformPostprocessingBuffers[i], nullptr);
        vkFreeMemory(device, uniformPostprocessingBuffersMemory[i], nullptr);
    }

    ImGui_ImplVulkan_Shutdown();

    vkDestroyDescriptorPool(device, graphicsDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, postprocessDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, postprocessDescriptorSetLayout, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, textureIndexBuffer, nullptr);
    vkFreeMemory(device, textureIndexBufferMemory, nullptr);

    vkDestroyBuffer(device, textureIndexOffsetBuffer, nullptr);
    vkFreeMemory(device, textureIndexOffsetBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, imageFences[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);

    vkDestroyPipeline(device, matrixComputePipeline, nullptr);
    vkDestroyPipelineLayout(device, matrixComputePipelineLayout, nullptr);

    vkDestroyPipeline(device, cullingComputePipeline, nullptr);
    vkDestroyPipelineLayout(device, cullingComputePipelineLayout, nullptr);

    for (const auto &val: postprocessPipelines | std::views::values) {
        vkDestroyPipeline(device, val, nullptr);
    }

    vkDestroyPipelineLayout(device, postprocessPipelineLayout, nullptr);

    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);

    vkDestroyCommandPool(device, matrixComputeCommandPool, nullptr);

    vkDestroyCommandPool(device, cullingComputeCommandPool, nullptr);

    vkDestroyCommandPool(device, postprocessCommandPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, drawCommandsBuffers[i], nullptr);
        vkFreeMemory(device, drawCommandsBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, drawCommandsSourceBuffers[i], nullptr);
        vkFreeMemory(device, drawCommandsSourceBuffersMemory[i], nullptr);
    }

    vkDestroyDevice(device, nullptr);

    if constexpr (enableValidationLayers) {
        Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
#pragma endregion


#pragma region Buffers
// Generic
void Engine::updateUniformBuffer() {
    const glm::mat4 view = glm::lookAt(camera.pos + glm::vec3(0), camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 proj = glm::perspective(camera.fov,  static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 4096.0f);
    proj[1][1] *= -1;

    ubo.view = view;
    ubo.proj = proj;
    ubo.count = mem.getTotalModelCount();

    umbo.view = view;
    umbo.proj = proj;
    umbo.count = mem.getTotalInstanceCount();

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    memcpy(uniformMatrixBuffersMapped[currentFrame], &umbo, sizeof(umbo));
}

void Engine::updateUniformPostprocessingBuffer() {
    upbo.time += static_cast<float>(delta.deltaTime);
    upbo.resolution = glm::vec2(swapchainExtent.width, swapchainExtent.height);
    memcpy(uniformPostprocessingBuffersMapped[currentFrame], &upbo, sizeof(upbo));
}

void Engine::normalizePlane(glm::vec4& plane) {
    const float length = glm::length(glm::vec3(plane));
    if (length > 0.0f) {
        plane /= length;
    }
}

glm::vec4 Engine::extractPlane(glm::mat4 viewProjection, const int a, const float sign) {
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

void Engine::updateCullingUniformBuffer() {
    ucbo.totalObjects = mem.getTotalInstanceCount();

    const glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f)); // z-up
    glm::mat4 proj = glm::perspective(camera.fov, static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 4096.0f);
    proj[1][1] *= -1;

    const glm::mat4 viewProjection = proj * view;

    ucbo.planes[0] = extractPlane(viewProjection, 0, +1); // right
    ucbo.planes[1] = extractPlane(viewProjection, 0, -1); // left
    ucbo.planes[2] = extractPlane(viewProjection, 1, +1); // top
    ucbo.planes[3] = extractPlane(viewProjection, 1, -1); // bottom
    ucbo.planes[4] = extractPlane(viewProjection, 2, +1); // far
    ucbo.planes[5] = extractPlane(viewProjection, 2, -1); // near

    memcpy(uniformCullingBuffersMapped[currentFrame], &ucbo, sizeof(ucbo));
}


// Matrices
void Engine::updateModelDataBuffer() {
    static size_t frame = 0;

    if (mem.dirty[2]) {
        auto* dst = static_cast<glm::vec4*>(modelDataBuffersMapped[currentFrame]);
        size_t i = 0;
        for (const auto& group : std::views::transform(std::views::values(mem.groups), &ModelGroup::instances)) {
            for (const auto& model_instance : group) {
                dst[i * 3 + 0] = model_instance.pos;
                dst[i * 3 + 1] = model_instance.rot;
                dst[i * 3 + 2] = model_instance.scl;
                i++;
            }
        }

        frame++;
        if (frame == MAX_FRAMES_IN_FLIGHT) mem.dirty[2] = false;
    }
}

void Engine::updateModelBuffer() {
    if (!initializedMbo) {
        mbo.models.clear();
        mbo.models.resize(mem.getTotalInstanceCount());
        for (const auto& object : modelBuffersMapped) {
            memcpy(object, mbo.models.data(), sizeof(glm::mat4) * mbo.models.size());
        }

        initializedMbo = true;
    }
}

void Engine::updateSingleModel(const std::string &name, const uint32_t instanceIndex, glm::mat4 matrix) {
    if (!initializedMbo) return;

    const auto scale = mem.groups[name].instances[instanceIndex].scl;
    matrix[0] = glm::normalize(matrix[0]) * scale.x;
    matrix[1] = glm::normalize(matrix[1]) * scale.y;
    matrix[2] = glm::normalize(matrix[2]) * scale.z;
    mbo.models[instanceIndex] = matrix;

    size_t modelIndex = 0;
    for (int i = 0; i < mem.indexedFiles.size(); ++i) {
        if (mem.indexedFiles[i] == name) modelIndex = mem.indexedFiles.size() - (i + 1);
    }

    const size_t matOffset = (modelIndex + instanceIndex) * sizeof(glm::mat4);
    const size_t sphereOffset = (modelIndex + instanceIndex) * sizeof(CullingData);


    for (void* mappedPtr : modelBuffersMapped) {
        void* targetAddress = static_cast<char*>(mappedPtr) + matOffset;

        memcpy(targetAddress, &matrix, sizeof(glm::mat4));
    }

    mcbo.cullingDatas[modelIndex].sphere = mem.groups[name].model->sphere + glm::vec4(matrix[3]);
    void* targetAddress = static_cast<char*>(modelCullingBufferMapped) + sphereOffset;
    memcpy(targetAddress, &mcbo.cullingDatas[modelIndex], sizeof(CullingData));
}


// Culling
void Engine::updateModelCullingBuffer() {
    if (mem.dirty[1]) {
        mcbo.cullingDatas.clear();
        mcbo.cullingDatas.reserve(mem.getTotalInstanceCount());

        uint16_t index = 0;
        for (const auto& group : std::views::transform(std::views::values(mem.groups), &ModelGroup::instances)) {
            for (const auto& model_instance : group) {
                glm::vec4 sphere = model_instance.mdl.lock()->sphere + model_instance.pos;
                sphere.w *= glm::compMax(model_instance.scl);
                mcbo.cullingDatas.emplace_back(sphere, index);
            }

            index++;
        }

        memcpy(modelCullingBufferMapped, mcbo.cullingDatas.data(), mcbo.cullingDatas.size() * sizeof(CullingData));

        mem.dirty[1] = false;
    }
}

void Engine::updateVisibleIndicesBuffer() {
    static bool initialized = false;

    if (!initialized) {
        vio.vi.clear();
        for (const auto& object : visibleIndicesBuffersMapped) {
            memcpy(object, vio.vi.data(), vio.vi.size() * sizeof(uint32_t));
        }
    }
}

void Engine::updateDrawCommands() {
    dcs.commands.clear();

    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;

    for (const auto& group : std::views::values(mem.groups)) {
        VkDrawIndexedIndirectCommand command{};

        const size_t instanceCount = group.instances.size();
        const uint32_t indexCount = ModelEntityManager::getIndexCount(group.model);

        command.firstInstance = firstInstance;
        command.instanceCount = instanceCount;

        command.firstIndex = firstIndex;
        command.indexCount = indexCount;
        command.vertexOffset = vertexOffset;

        firstIndex += indexCount;
        vertexOffset += ModelEntityManager::getVertexCount(group.model);
        firstInstance += instanceCount;
        dcs.commands.emplace_back(command);
    }
}

void Engine::updateTextureIndexBuffer() {
    static bool initialized = false;

    if (!initialized) {
        size_t i = 0;
        const size_t count = mem.getTotalModelCount();
        uint32_t currentGlobalVertexOffset = 0;

        tio.indices.resize(count);

        for (const auto& model : mem.groups | std::views::values | std::views::transform(&ModelGroup::model)) {
            auto& textures = model->textures;

            if (textures.empty()) {
                tio.indices[i].firstIndex = 0;
                tio.indices[i].indexCount = 0;
            } else {
                tio.indices[i].firstIndex = textures[0].index;
                tio.indices[i].indexCount = textures.size();
                tio.indices[i].pad1 = currentGlobalVertexOffset;

                //std::cout << "==================" << std::endl;
                //std::cout << "First index: " << textures[0].index << std::endl;
                //std::cout << "Tex count: " << tio.indices[i].indexCount << std::endl;
                //std::cout << "Padding index: " << tio.indices[i].pad1 << std::endl;
                //std::cout << "==================" << std::endl;
                for (const auto & texture : textures) {
                    const uint32_t offset = texture.indexOffset;
                    tioo.indexOffsets.emplace_back(offset);
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

        //std::cout << "Index offsets count: " << tioo.indexOffsets.size() << std::endl;
        //std::cout << "Index offsets2 count: " << tio.indices.size() << std::endl;

        memcpy(textureIndexBufferMapped, tio.indices.data(), sizeof(Index) * tio.indices.size());
        memcpy(textureIndexOffsetBufferMapped, tioo.indexOffsets.data(), tioo.indexOffsets.size() * sizeof(uint32_t));

        initialized = true;
    }
}
#pragma endregion

void Engine::recreatePostprocessingPipeline(const std::string& filename) {
    PipelineCreation::createPostprocessPipeline(device, physicalDevice, postprocessPipelineLayout, postprocessPipelines[filename], postprocessDescriptorSetLayout, swapchainImageFormat, filename);
}

#pragma region Initialization
// Initialization of engine and its counterparts
void Engine::initialize(SDL_Window* sdl_window) {
    const auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::filesystem::path> dirtyShaders = Shaders::getShadersToCompile();
    for (auto& dirtyShader : dirtyShaders) {
        auto shader = Shaders::compileShader(dirtyShader);
        Shaders::saveShaderToFile(Tools::getShaderPath() + "compiled/" + dirtyShader.lexically_relative(Tools::getShaderPath()).replace_extension(".spv").string(), shader);
    }

    this->window = sdl_window;

    // Engine related stuff
    sid = Random::randomNum<uint64_t>(1000000000,9999999999);
    LOGGER = Logger("engine.hpp");

    initializeVulkan();
    initializeImGui(sdl_window);

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;

    desiredFrameRate = 200.0;
    sleepTimeTotalSeconds = 1.0 / desiredFrameRate;

    // Success!?
    LOGGER.success("2g43s engine STARTED successfully in ${} seconds!", duration.count());
    initialized = true;
}

// Creating Vulkan instance
void Engine::createInstance() {
    // Check if validation layers available when requested
    if (enableValidationLayers && !Debug::checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    // App info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "2g43s";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "2g43s Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    // Instance info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Vulkan extensions
    const auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layers initialization
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if constexpr (enableValidationLayers) {
        VkValidationFeaturesEXT validationFeatures = {};
        VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
        validationFeatures.pEnabledValidationFeatures = enables;
        validationFeatures.enabledValidationFeatureCount = 1;
        validationFeatures.pNext = &debugCreateInfo;

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        Debug::populateDebugMessengerCreateInfo(debugCreateInfo);
        debugCreateInfo.pNext = nullptr;

        createInfo.pNext = &debugCreateInfo;

    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Trying to create VK_KHR_xlib_surfacevulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void Engine::initializeImGui(SDL_Window* sdl_window) {
    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Скругляем углы для более современного вида
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.01f, 0.01f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.02f, 0.02f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.03f, 0.03f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.40f, 0.00f, 0.00f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.58f, 0.00f, 0.00f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.35f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.51f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.71f, 0.00f, 0.00f, 1.00f);

    colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.51f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.86f, 0.00f, 0.00f, 1.00f);

    colors[ImGuiCol_Button]                 = ImVec4(0.44f, 0.00f, 0.00f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.74f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.98f, 0.06f, 0.06f, 1.00f);

    colors[ImGuiCol_Header]                 = ImVec4(0.70f, 0.00f, 0.00f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.70f, 0.00f, 0.00f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.48f, 0.00f, 0.00f, 1.00f);

    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.75f, 0.10f, 0.10f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.75f, 0.10f, 0.10f, 1.00f);

    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.47f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.66f, 0.00f, 0.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.81f, 0.00f, 0.00f, 0.95f);

    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],     colors[ImGuiCol_TitleBg], 0.40f);

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF((Tools::getCorePath() + "fonts/opensans.ttf").c_str(), 18.0f);
    io.FontGlobalScale = 1.0f;

    ImGui_ImplSDL3_InitForVulkan(sdl_window);

    const QueueFamilyIndices indices = Queue::findQueueFamilies(physicalDevice, surface);

    ImGui_ImplVulkan_InitInfo info{};

    info.ApiVersion = VK_API_VERSION_1_4;
    info.Instance = instance;
    info.PhysicalDevice = physicalDevice;
    info.Device = device;
    info.Queue = graphicsQueue;
    info.QueueFamily = indices.graphicsFamily.value();
    info.DescriptorPool = graphicsDescriptorPool;
    info.DescriptorPoolSize = 0;
    info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    info.ImageCount = MAX_FRAMES_IN_FLIGHT;
    info.UseDynamicRendering = true;

    ImGui_ImplVulkan_PipelineInfo pipeline_info{};
    pipeline_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipeline_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_info.PipelineRenderingCreateInfo.depthAttachmentFormat = Helper::findDepthFormat(physicalDevice);
    pipeline_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;
    pipeline_info.PipelineRenderingCreateInfo.stencilAttachmentFormat = Helper::findDepthFormat(physicalDevice);

    info.PipelineInfoMain = pipeline_info;

    ImGui_ImplVulkan_Init(&info);
}

void Engine::initializeOffscreenImages(const uint32_t width, const uint32_t height) {
    offscreenImages.resize(MAX_FRAMES_IN_FLIGHT);
    offscreenImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    offscreenImagesMemory.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        Images::createImage(device, physicalDevice, width, height, VK_FORMAT_R16G16B16A16_SFLOAT , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenImages[i], offscreenImagesMemory[i]);
        offscreenImageViews[i] = Images::createImageView(device, offscreenImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Engine::initializePostprocessPipelines() {
    const std::string path = Tools::getCompiledShaderPath() + "postprocessing/";

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) return;

    PipelineCreation::createPostprocessPipelineLayout(device, postprocessPipelineLayout, postprocessDescriptorSetLayout);

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            PipelineCreation::createPostprocessPipeline(device, physicalDevice, postprocessPipelineLayout, postprocessPipelines[filename], postprocessDescriptorSetLayout, swapchainImageFormat, filename);
        }
    }
}

void Engine::initializeVulkan() {
    delta = DeltaManager();

    // Basic things
    createInstance();
    setupDebugMessenger();

    // SDL3 surface
    Surface::createSurface(surface, window, instance);

    // Devices
    PhysicalDevice::pickPhysicalDevice(physicalDevice, instance, surface);
    PhysicalDevice::getSubgroupProperties(physicalDevice, subgroupProperties);
    LogicalDevice::createLogicalDevice(device, physicalDevice, graphicsQueue, presentQueue, surface);


    LOGGER.info("Subgroup size: ${}", subgroupProperties.subgroupSize);

    // Swapchain
    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    initializeOffscreenImages(swapchainExtent.width, swapchainExtent.height);

    // Desccriptor
    Descriptor::createGraphicsDescriptorSetLayout(device, graphicsDescriptorSetLayout);
    Descriptor::createPostprocessDescriptorSetLayout(device, postprocessDescriptorSetLayout);

    PipelineCreation::createGraphicsPipeline(device, physicalDevice, graphicsPipelineLayout, graphicsPipeline, graphicsDescriptorSetLayout, swapchainImageFormat);
    PipelineCreation::createMatrixComputePipeline(device, matrixComputePipelineLayout, matrixComputePipeline);
    PipelineCreation::createCullingComputePipeline(device, cullingComputePipelineLayout, cullingComputePipeline);

    initializePostprocessPipelines();
    Buffers::createGenericBuffers(device, physicalDevice, uniformPostprocessingConstants, uniformPostprocessingBuffers, uniformPostprocessingBuffersMemory, uniformPostprocessingBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformPostprocessingBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    Command::createCommandPool(device, physicalDevice, graphicsCommandPool, surface);
    Helper::createDepthResources(device, physicalDevice, depthImage, depthImageMemory, depthImageView, swapchainExtent);

    Images::createTextureImage(device, graphicsCommandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, textureImage, textureImageMemory, "/home/down1/2g43s/core/textures/missingno.ktx2", VK_FORMAT_BC7_UNORM_BLOCK);
    Images::createTextureImageView(device, textureImage, textureImageView, VK_FORMAT_BC7_UNORM_BLOCK);
    Images::createTextureSampler(device, physicalDevice, textureSampler);

    mem.scene();
    ModelBus::loadModelTextures(mem.groups, device, graphicsCommandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory);

    Buffers::createVertexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, mem);
    Buffers::createIndexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, indexBuffer, indexBufferMemory, mem);

    // Generic
    Buffers::createGenericBuffers(device, physicalDevice, uniformConstants, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffers::createGenericBuffers(device, physicalDevice, uniformMatrixConstants, uniformMatrixBuffers, uniformMatrixBuffersMemory, uniformMatrixBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffers::createGenericBuffers(device, physicalDevice, atomicCounterConstants, atomicCounterBuffers, atomicCounterBuffersMemory, atomicCounterBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Matrices
    Buffers::createGenericBuffers(device, physicalDevice, modelConstants, modelBuffers, modelBuffersMemory, modelBuffersMapped, MAX_FRAMES_IN_FLIGHT, mem.getModelBufferSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffers::createGenericBuffers(device, physicalDevice, modelDataConstants, modelDataBuffers, modelDataBuffersMemory, modelDataBuffersMapped, MAX_FRAMES_IN_FLIGHT, mem.getModelDataBufferSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Culling
    Buffers::createGenericBuffers(device, physicalDevice, uniformCullingConstants, uniformCullingBuffers, uniformCullingBuffersMemory, uniformCullingBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(UniformCullingBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffers::createGenericBuffers(device, physicalDevice, visibleIndicesConstants, visibleIndicesBuffers, visibleIndicesBuffersMemory, visibleIndicesBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(uint32_t) * mem.getTotalInstanceCount(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    Buffers::createGenericBuffer(device, physicalDevice, modelCullingConstant, modelCullingBuffer, modelCullingBufferMemory, modelCullingBufferMapped, sizeof(CullingData) * mem.getTotalInstanceCount(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    updateDrawCommands();
    Buffers::createGenericBuffers(device, physicalDevice, drawCommandsSourceConstants, drawCommandsSourceBuffers, drawCommandsSourceBuffersMemory, drawCommandsSourceBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(VkDrawIndexedIndirectCommand) * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Buffers::createGenericBuffers(device, physicalDevice, drawCommandsConstants, drawCommandsBuffers, drawCommandsBuffersMemory, drawCommandsBuffersMapped, MAX_FRAMES_IN_FLIGHT, sizeof(VkDrawIndexedIndirectCommand) * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        memcpy(drawCommandsSourceBuffersMapped[i], dcs.commands.data(), sizeof(VkDrawIndexedIndirectCommand) * dcs.commands.size());
        //memcpy(drawCommandsBuffersMapped[i], dc.commands.data(), sizeof(VkDrawIndexedIndirectCommand) * dc.commands.size());
    }


    Buffers::createGenericBuffer(device, physicalDevice, textureIndexConstant, textureIndexBuffer, textureIndexBufferMemory, textureIndexBufferMapped, sizeof(uint32_t) * 4 * 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffers::createGenericBuffer(device, physicalDevice, textureIndexOffsetConstant, textureIndexOffsetBuffer, textureIndexOffsetBufferMemory, textureIndexOffsetBufferMapped, sizeof(uint32_t) * 128, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // Descriptor
    Descriptor::createGraphicsDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorPool);
    Descriptor::createGraphicsDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout, graphicsDescriptorPool, graphicsDescriptorSets, uniformBuffers, textureSampler, modelBuffers, visibleIndicesBuffers, mem, textureImageView, textureIndexBuffer, textureIndexOffsetBuffer);

    Descriptor::createPostprocessDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, postprocessDescriptorPool);
    Descriptor::createPostprocessDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, postprocessDescriptorSetLayout, postprocessDescriptorPool, postprocessDescriptorSets, offscreenImageViews, depthImageView, textureSampler, uniformPostprocessingBuffers);

    Buffers::createCommandBuffer(device, graphicsCommandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT);
    Sync::createSemaphores(device, MAX_FRAMES_IN_FLIGHT, imageAvailableSemaphores, renderFinishedSemaphores);
    Sync::createFences(device, MAX_FRAMES_IN_FLIGHT, imageFences, inFlightFences);
}
#pragma endregion


#pragma region Swapchain

void Engine::cleanupOffscreenImages() const {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyImageView(device, offscreenImageViews[i], nullptr);
        vkDestroyImage(device, offscreenImages[i], nullptr);
        vkFreeMemory(device, offscreenImagesMemory[i], nullptr);
    }
}

void Engine::cleanupSwapchain() const {
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);

    for (auto& swapchainImageView : swapchainImageViews) {
        vkDestroyImageView(device, swapchainImageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Engine::recreateSwapchain() {
    vkDeviceWaitIdle(device);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    cleanupOffscreenImages();
    cleanupSwapchain();

    initializeOffscreenImages(width, height);
    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    Helper::createDepthResources(device, physicalDevice, depthImage, depthImageMemory, depthImageView, swapchainExtent);

    Descriptor::updatePostprocessDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, postprocessDescriptorSets, offscreenImageViews, depthImageView, textureSampler);
}
#pragma endregion


#pragma region Helper
// Setup debug messenger
void Engine::setupDebugMessenger() {
    if constexpr (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    Debug::populateDebugMessengerCreateInfo(createInfo);

    if (Debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

// Get SDL extensions
std::vector<const char*> Engine::getRequiredExtensions() {
    uint32_t extensionCount = 0;
    auto SDL_extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    std::vector extensions(SDL_extensions, SDL_extensions + extensionCount);

    // If validation layers enabled when add them to extensions vector
    if constexpr (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    extensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

    return extensions;
}
#pragma endregion

#endif //ENGINE_CPP