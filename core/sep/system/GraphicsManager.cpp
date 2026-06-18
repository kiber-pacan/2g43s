//
// Created by down1 on 25.02.2026.
//

#include "GraphicsManager.hpp"

#include <algorithm>
#include <stdexcept>

#include "Barrier.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "PostprocessPushConstants.hpp"
#include "VertexPushConstants.hpp"

#include "DeltaManager.hpp"
#include "SwapchainManager.hpp"
#include "ModelEntityManager.hpp"
#include "BufferManager.hpp"
#include "CullingPushConstants.hpp"
#include "Descriptor.hpp"
#include "imgui_internal.h"
#include "MatrixPushConstants.hpp"
#include "ModelBus.hpp"
#include "PipelineCreation.hpp"
#include "Sync.hpp"
#include "Tools.hpp"

#pragma region Internal
void GraphicsManager::drawImGui(const VkCommandBuffer& commandBuffer) {
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
    ImGui::Value("Framerate", static_cast<int>(delta->frameRate));
    ImGui::Value("Total models", static_cast<int>(modelEntityManager->modelsCount()));
    ImGui::Value("Total instances", static_cast<int>(modelEntityManager->getTotalInstanceCount()));

    int desiredFps = static_cast<int>(*desiredFrameRate);

    if (ImGui::BeginTable("fps_table", 2)) {
        ImGui::TableNextRow();

        const auto label = "Desired FPS";
        const float textWidth = ImGui::CalcTextSize(label).x;
        const float fpsWidth = ImGui::CalcTextSize(std::to_string(*desiredFrameRate).c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;

        ImGui::TableSetColumnIndex(0);
        ImGui::SetNextItemWidth(textWidth);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(label);

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(fpsWidth);
        if (ImGui::InputInt("##desiredFps", &desiredFps, 0)) {
            *desiredFrameRate = desiredFps;
            *sleepTimeTotalNS = 1.0 / std::clamp(*desiredFrameRate, 15.0, std::numeric_limits<double>::max()) * 1'000'000'000;
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

void GraphicsManager::recordCommandBuffer(VkPipeline postprocessPipeline, uint32_t imageIndex) {
    VkCommandBuffer& commandBuffer = bufferManager->commandBuffers[currentFrame];
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }


    #pragma region Cleanup
    vkCmdFillBuffer(commandBuffer, bufferManager->atomicCounterBuffers[currentFrame], 0, sizeof(uint32_t), 0); // Clear atomic counter
    for (int i = 0; i < modelEntityManager->getTotalModelCount(); i++) {
        vkCmdFillBuffer(commandBuffer, bufferManager->drawCommandsSourceBuffers[currentFrame], offsetof(VkDrawIndexedIndirectCommand, instanceCount) + sizeof(VkDrawIndexedIndirectCommand) * i, 4, 0); // Clear 4 bites with offset of 4
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
        matrixConstants.mdb = bufferManager->modelDataConstants[currentFrame];
        matrixConstants.mb = bufferManager->modelConstants[currentFrame];
        matrixConstants.umbo = bufferManager->uniformMatrixConstants[currentFrame];

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

        const size_t totalInstances = modelEntityManager->getTotalInstanceCount();
        uint32_t groupCount = (totalInstances + workgroupSize - 1) / workgroupSize;

        vkCmdDispatch(commandBuffer, groupCount, 1, 1);
        Barrier vibBarrier(commandBuffer);
        vibBarrier.buffer(
            bufferManager->modelDataBuffers[currentFrame],
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
        cullingConstants.mcb = bufferManager->modelCullingConstant;
        cullingConstants.vib = bufferManager->visibleIndicesConstants[currentFrame];
        cullingConstants.dcsb = bufferManager->drawCommandsSourceConstants[currentFrame];
        cullingConstants.dcb = bufferManager->drawCommandsConstants[currentFrame];
        cullingConstants.ucbo =bufferManager-> uniformCullingConstants[currentFrame];
        cullingConstants.counter = bufferManager->atomicCounterConstants[currentFrame];

        vkCmdPushConstants(
        commandBuffer,
            cullingComputePipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(CullingPushConstants),
            &cullingConstants
            );

        constexpr uint32_t workgroupSize1 = 128;
        const size_t totalInstances = modelEntityManager->getTotalInstanceCount();
        uint32_t groupCount = (totalInstances + workgroupSize1 - 1) / workgroupSize1;

        vkCmdDispatch(commandBuffer, groupCount, 1, 1);
        Barrier vibBarrier(commandBuffer);
        vibBarrier.buffer(
            bufferManager->atomicCounterBuffers[currentFrame],
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            0, VK_WHOLE_SIZE
        ).buffer(
            bufferManager->visibleIndicesBuffers[currentFrame],
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            0, VK_WHOLE_SIZE
        ).buffer(
            bufferManager->drawCommandsSourceBuffers[currentFrame],
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
    colorAttachment.imageView = swapchainManager->offscreenImageViews[currentFrame];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValues[0];

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = swapchainManager->depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, swapchainManager->swapchainExtent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    #pragma endregion

    // Transition image layouts so we can write in it
    #pragma region initialImageLayoutTransition
    Barrier barrier(commandBuffer);

    barrier.image(
    swapchainManager->depthImage,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_ACCESS_2_NONE, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
    ).image(
    swapchainManager->offscreenImages[currentFrame],
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
    vertexConstants.ubo = bufferManager->uniformConstants[currentFrame];
    vertexConstants.mb = bufferManager->modelConstants[currentFrame];
    vertexConstants.vib = bufferManager->visibleIndicesConstants[currentFrame];
    vertexConstants.ti = bufferManager->textureIndexConstant;
    vertexConstants.tio = bufferManager->textureIndexOffsetConstant;

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
    viewport.width = static_cast<float>(swapchainManager->swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainManager->swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainManager->swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {bufferManager->vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, bufferManager->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    //vkCmdDrawIndexedIndirectCount(commandBuffer, bufferManager->drawCommandsSourceBuffers[currentFrame], 0, bufferManager->atomicCounterBuffers[currentFrame], 0, 1024, sizeof(VkDrawIndexedIndirectCommand));
    //vkCmdDrawIndexedIndirect(commandBuffer, drawCommandsSourceBuffers[currentFrame], 0, static_cast<uint32_t>(mdlBus.getTotalModelCount()), sizeof(VkDrawIndexedIndirectCommand));
    vkCmdDrawIndexedIndirect(commandBuffer, bufferManager->drawCommandsSourceBuffers[currentFrame], 0, static_cast<uint32_t>(modelEntityManager->getTotalModelCount()), sizeof(VkDrawIndexedIndirectCommand));


    vkCmdEndRendering(commandBuffer);
    #pragma endregion

    // Transition Offscreen image layout so we can read it from shaders
    #pragma region offscreenLayoutTransition
    Barrier midBarrier(commandBuffer);
    barrier.image(
        swapchainManager->offscreenImages[currentFrame],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
        ).image(
        swapchainManager->depthImage,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
        ).image(
        swapchainManager->swapchainImages[imageIndex],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
        ).apply();
    #pragma endregion

    #pragma region screenRenderInfo
    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = swapchainManager->swapchainImageViews[imageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfoScreen{};
    renderingInfoScreen.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfoScreen.renderArea = {{0, 0}, swapchainManager->swapchainExtent};
    renderingInfoScreen.layerCount = 1;
    renderingInfoScreen.colorAttachmentCount = 1;
    renderingInfoScreen.pColorAttachments = &colorAttachmentInfo;
    #pragma endregion

    // Render face with offscreen image so we can do postprocessing
    #pragma region renderScreen
    vkCmdBeginRendering(commandBuffer, &renderingInfoScreen);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postprocessPipeline);

    PostprocessPushConstants postprocessingConstants{};
    postprocessingConstants.mpbo = bufferManager->uniformPostprocessingConstants[currentFrame];

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
    swapchainManager->swapchainImages[imageIndex],
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

void GraphicsManager::waitForFence(VkFence fence) const {
    Logger logger("waitForFences()");

    const auto result = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    if (result != VK_SUCCESS) {
        logger.error("ERROR!");
    } else {
        vkResetFences(device, 1, &fence);
    }
}

// Create pipeline for each postprocessing shader
void GraphicsManager::initializePostprocessPipelines() {
    const std::string path = Tools::getCompiledShaderPath() + "postprocessing/";

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) return;

    PipelineCreation::createPostprocessPipelineLayout(device, postprocessPipelineLayout, postprocessDescriptorSetLayout);

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            PipelineCreation::createPostprocessPipeline(device, physicalDevice, postprocessPipelineLayout, postprocessPipelines[filename], postprocessDescriptorSetLayout, swapchainManager->swapchainImageFormat, filename);
        }
    }
}

void GraphicsManager::initializePipelines() {
    Descriptor::createGraphicsDescriptorSetLayout(device, graphicsDescriptorSetLayout);
    Descriptor::createPostprocessDescriptorSetLayout(device, postprocessDescriptorSetLayout);

    PipelineCreation::createGraphicsPipeline(device, physicalDevice, graphicsPipelineLayout, graphicsPipeline, graphicsDescriptorSetLayout, swapchainManager->swapchainImageFormat);
    PipelineCreation::createMatrixComputePipeline(device, matrixComputePipelineLayout, matrixComputePipeline);
    PipelineCreation::createCullingComputePipeline(device, cullingComputePipelineLayout, cullingComputePipeline);

    initializePostprocessPipelines();

    Command::createCommandPool(device, physicalDevice, graphicsCommandPool, swapchainManager->surface);
    Helper::createDepthResources(device, physicalDevice, swapchainManager->depthImage, swapchainManager->depthImageMemory, swapchainManager->depthImageView, swapchainManager->swapchainExtent);
}

void GraphicsManager::initializeDescriptors() {
    Descriptor::createGraphicsDescriptorPool(device, graphicsDescriptorPool, MAX_FRAMES_IN_FLIGHT);
    Descriptor::createGraphicsDescriptorSets(device, graphicsDescriptorSetLayout, graphicsDescriptorPool, graphicsDescriptorSets, swapchainManager->textureSampler, missingnoTextureImageView, *modelEntityManager, MAX_FRAMES_IN_FLIGHT);

    Descriptor::createPostprocessDescriptorPool(device, postprocessDescriptorPool, MAX_FRAMES_IN_FLIGHT);
    Descriptor::createPostprocessDescriptorSets(device, postprocessDescriptorSetLayout, postprocessDescriptorPool, postprocessDescriptorSets, swapchainManager->offscreenImageViews, swapchainManager->depthImageView, swapchainManager->textureSampler, bufferManager->uniformPostprocessingBuffers, MAX_FRAMES_IN_FLIGHT);

    BuffersRegistry::createCommandBuffer(device, graphicsCommandPool, bufferManager->commandBuffers, MAX_FRAMES_IN_FLIGHT);
    Sync::createSemaphores(device, MAX_FRAMES_IN_FLIGHT, imageAvailableSemaphores, renderFinishedSemaphores);
    Sync::createFences(device, MAX_FRAMES_IN_FLIGHT, imageFences, inFlightFences);
}

void GraphicsManager::initializeTextures() {
    // Missingno texture
    Images::createTextureImage(device, graphicsCommandPool, graphicsQueue, physicalDevice, bufferManager->stagingBuffer, bufferManager->stagingBufferMemory, missingnoTextureImage, missingnoTextureImageMemory, std::string{PROJECT_ROOT} + "core/textures/missingno.ktx2", VK_FORMAT_BC7_UNORM_BLOCK);
    Images::createTextureImageView(device, missingnoTextureImage, missingnoTextureImageView, VK_FORMAT_BC7_UNORM_BLOCK);

    ModelBus::loadModelTextures(modelEntityManager->groups, device, graphicsCommandPool, graphicsQueue, physicalDevice, bufferManager->stagingBuffer, bufferManager->stagingBufferMemory);

    Images::createTextureSampler(device, physicalDevice, swapchainManager->textureSampler);
}

void GraphicsManager::initializeImGui() {
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

    ImGui_ImplSDL3_InitForVulkan(swapchainManager->window);

    const QueueFamilyIndices indices = Queue::findQueueFamilies(physicalDevice, swapchainManager->surface);

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
    pipeline_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainManager->swapchainImageFormat;
    pipeline_info.PipelineRenderingCreateInfo.stencilAttachmentFormat = Helper::findDepthFormat(physicalDevice);

    info.PipelineInfoMain = pipeline_info;

    ImGui_ImplVulkan_Init(&info);
}
#pragma endregion

void GraphicsManager::initialize() {
    initializePipelines();
    bufferManager->createBuffers(graphicsQueue, graphicsCommandPool);
    initializeTextures();
    initializeDescriptors();
    initializeImGui();
}

void GraphicsManager::recreatePostprocessingPipeline(const std::string& filename) {
    PipelineCreation::createPostprocessPipeline(device, physicalDevice, postprocessPipelineLayout, postprocessPipelines[filename], postprocessDescriptorSetLayout, swapchainManager->swapchainImageFormat, filename);
}

// Draw
void GraphicsManager::drawFrame()  {
    waitForFence(imageFences[currentFrame]);
    waitForFence(inFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchainManager->swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchainManager->recreateSwapchain(postprocessDescriptorSets);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    bufferManager->updateUniformBuffer(currentFrame);
    bufferManager->updateUniformPostprocessingBuffer(currentFrame, *delta);
    bufferManager->updateCullingUniformBuffer(currentFrame);
    bufferManager->updateModelCullingBuffer();
    bufferManager->updateModelDataBuffer(currentFrame);
    bufferManager->updateModelBuffer();
    bufferManager->updateTextureIndexBuffer();

    std::function<void(VkCommandBuffer&)> imGui = [this](const VkCommandBuffer& commandBuffer) { drawImGui(commandBuffer); };

    recordCommandBuffer(postprocessPipelines[selectedShader], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &bufferManager->commandBuffers[currentFrame];

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

    const VkSwapchainKHR swapchains[] = { swapchainManager->swapchain };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pNext = &imageFenceInfo;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || *framebufferResized) {
        *framebufferResized = false;
        swapchainManager->recreateSwapchain(postprocessDescriptorSets);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsManager::cleanupTextures() const {
    vkDestroySampler(device, swapchainManager->textureSampler, nullptr);
    vkDestroyImageView(device, missingnoTextureImageView, nullptr);

    vkDestroyImage(device, missingnoTextureImage, nullptr);
    vkFreeMemory(device, missingnoTextureImageMemory, nullptr);

    for (auto& textures : modelEntityManager->groups
        | std::views::transform(&ModelGroup::model)
        | std::views::transform(&ParsedModel::textures)
    ) {
        for (const auto& texture : textures) {
            vkDestroyImageView(device, texture.textureImageView, nullptr);
            vkDestroyImage(device, texture.textureImage, nullptr);
            vkFreeMemory(device, texture.textureImageMemory, nullptr);
        }
    }
}

void GraphicsManager::cleanup() const {
    vkDestroyDescriptorPool(device, graphicsDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, postprocessDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, postprocessDescriptorSetLayout, nullptr);

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
}
