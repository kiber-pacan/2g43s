#ifndef ENGINE_CPP
#define ENGINE_CPP

#include "Engine.hpp"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>


#include "Command.hpp"
#include "Descriptor.hpp"
#include "Helper.hpp"
#include "PipelineCreation.hpp"
#include "device/LogDevice.hpp"
#include "device/PhysDevice.hpp"
#include "graphics/Buffers.hpp"
#include "window/Surface.hpp"


#pragma region Main
// Method for initializing and running engine
void Engine::init(SDL_Window* window) {
    initialize(window);
}

void Engine::drawImGui(const VkCommandBuffer& commandBuffer) const {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();

    ImGui::NewFrame();

    ImGui::Begin("Stats");
    ImGui::SetNextWindowSize(ImVec2(200,160), ImGuiCond_Appearing);

    // FPS
    const std::string fps = "fps: " + std::to_string(deltaT->fps);
    ImGui::TextUnformatted(fps.c_str());

    // MDL BUS
    const std::string totalModels = "Total models: " + std::to_string(mdlBus.modelsCount());
    const std::string totalInstances = "Total instances: " + std::to_string(mdlBus.getTotalInstanceCount());

    ImGui::TextUnformatted(totalModels.c_str());
    ImGui::TextUnformatted(totalInstances.c_str());

    ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

// Draw
void Engine::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame, swapchainExtent, uniformBuffersMapped, ubo, camera);
    updateCullingUniformBuffer(currentFrame, swapchainExtent, uniformCullingBuffersMapped, ucbo, camera, mdlBus);
    updateModelCullingBuffer(modelCullingBufferMapped, mcbo, mdlBus);
    updateModelDataBuffer(currentFrame, modelDataBuffersMapped, mdlBus);
    updateModelBuffer(modelBuffersMapped, mbo);

    std::function<void(VkCommandBuffer&)> imGui = [this](const VkCommandBuffer& commandBuffer) { drawImGui(commandBuffer); };

    Command::recordCommandBuffer(
        device, physicalDevice, commandBuffers[currentFrame], imageIndex,
        swapchainExtent, vertexBuffer,
        indexBuffer, graphicsPipeline, matrixComputePipeline,
        cullingComputePipeline, graphicsPipelineLayout,
        matrixComputePipelineLayout, cullingComputePipelineLayout,
        graphicsDescriptorSets, matrixComputeDescriptorSets,
        cullingComputeDescriptorSets, currentFrame, clear_color, mdlBus,
        modelDataBuffers, drawCommandsBuffer, MAX_FRAMES_IN_FLIGHT,
        matrixDirty, atomicCounterBuffers, visibleIndicesBuffers,
        imGui,
        swapchainImageViews, depthImageView, depthImage, swapchainImages
    );

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    //vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

    presentInfo.pImageIndices = &imageIndex;

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

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
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
        vkFreeMemory(device, visibleIndicesMemory[i], nullptr);
    }

    vkDestroyBuffer(device, modelCullingBuffer, nullptr);
    vkFreeMemory(device, modelCullingBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformCullingBuffers[i], nullptr);
        vkFreeMemory(device, uniformCullingBuffersMemory[i], nullptr);
    }

    ImGui_ImplVulkan_Shutdown();

    vkDestroyDescriptorPool(device, graphicsDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, matrixComputeDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, matrixComputeDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(device, cullingComputeDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, cullingComputeDescriptorSetLayout, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);

    vkDestroyPipeline(device, matrixComputePipeline, nullptr);
    vkDestroyPipelineLayout(device, matrixComputePipelineLayout, nullptr);

    vkDestroyPipeline(device, cullingComputePipeline, nullptr);
    vkDestroyPipelineLayout(device, cullingComputePipelineLayout, nullptr);

    vkDestroyCommandPool(device, graphicsCommandPool, nullptr);

    vkDestroyCommandPool(device, matrixComputeCommandPool, nullptr);

    vkDestroyCommandPool(device, cullingComputeCommandPool, nullptr);


    vkDestroyBuffer(device, drawCommandsBuffer, nullptr);
    vkFreeMemory(device, drawCommandsBufferMemory, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
#pragma endregion


#pragma region Buffers
// Generic
void Engine::updateUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformBuffersMapped, UniformBufferObject& ubo, const Camera& camera) {
    ubo.view = glm::lookAt(camera.pos + glm::vec3(0), camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(camera.fov,  static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 4096.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void Engine::normalizePlane(glm::vec4& plane) {
    float length = glm::length(glm::vec3(plane));
    if (length > 0.0f) {
        plane /= length;
    }
}

glm::vec4 Engine::extractPlane(glm::mat4 viewProjection, int a, float sign) {
    glm::vec4 plane(
        viewProjection[0][3] - sign * viewProjection[0][a],
        viewProjection[1][3] - sign * viewProjection[1][a],
        viewProjection[2][3] - sign * viewProjection[2][a],
        viewProjection[3][3] - sign * viewProjection[3][a]
    );

    float length = glm::length(glm::vec3(plane));
    if (length > 0.0f) plane /= length;

    return plane;
}

void Engine::updateCullingUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformCullingBuffersMapped, UniformCullingBufferObject& ucbo, const Camera& camera, const ModelBus& mdlBus) {
    ucbo.totalObjects = mdlBus.getTotalInstanceCount();

    glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f)); // z-up
    glm::mat4 proj = glm::perspective(camera.fov, static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 4096.0f);
    proj[1][1] *= -1;

    glm::mat4 viewProjection = proj * view;

    ucbo.planes[0] = extractPlane(viewProjection, 0, +1); // right
    ucbo.planes[1] = extractPlane(viewProjection, 0, -1); // left
    ucbo.planes[2] = extractPlane(viewProjection, 1, +1); // top
    ucbo.planes[3] = extractPlane(viewProjection, 1, -1); // bottom
    ucbo.planes[4] = extractPlane(viewProjection, 2, +1); // far
    ucbo.planes[5] = extractPlane(viewProjection, 2, -1); // near

    memcpy(uniformCullingBuffersMapped[currentFrame], &ucbo, sizeof(ucbo));
}


// Matrices
void Engine::updateModelDataBuffer(const uint32_t currentFrame, const std::vector<void*>& modelDataBuffersMapped, const ModelBus& mdlBus) {
    static bool dirtyMDBO = true;
    static size_t frame = 0;

    if (dirtyMDBO) {
        auto* dst = static_cast<glm::vec4*>(modelDataBuffersMapped[currentFrame]);
        size_t i = 0;
        for (const auto& group : std::views::transform(std::views::values(mdlBus.groups_map), &ModelGroup::instances)) {
            for (const auto& instance : group) {
                dst[i * 3 + 0] = instance.pos;
                dst[i * 3 + 1] = instance.rot;
                dst[i * 3 + 2] = instance.scl;
                i++;
            }
        }

        frame++;
        if (frame == MAX_FRAMES_IN_FLIGHT) dirtyMDBO = false;
    }
}

void Engine::updateModelBuffer(const std::vector<void*>& modelBuffersMapped, ModelBufferObject& mbo) {
        static bool initialized = false;

        if (!initialized) {
            mbo.mdls.clear();
            for (auto& object : modelBuffersMapped) {
                memcpy(object, mbo.mdls.data(), mbo.mdls.size() * sizeof(glm::mat4));
            }
        }
    }


// Culling
void Engine::updateModelCullingBuffer(void*& modelCullingBufferMapped, ModelCullingBufferObject& mcbo, const ModelBus& mdlBus) {
    static bool initialized = false;

    if (!initialized) {
        mcbo.spheres.clear();
        mcbo.spheres.reserve(mdlBus.getTotalInstanceCount());

        for (const auto& group : std::views::transform(std::views::values(mdlBus.groups_map), &ModelGroup::instances)) {
            for (const auto& instance : group) {
                mcbo.spheres.emplace_back(instance.sfr);
            }
        }

        memcpy(modelCullingBufferMapped, mcbo.spheres.data(), mcbo.spheres.size() * sizeof(glm::vec4));

        initialized = true;
    }
}

void Engine::updateVisibleIndicesBuffer(const std::vector<void*>& visibleIndicesBuffersMapped, VisibleIndicesBufferObject& vio) {
    static bool initialized = false;

    if (!initialized) {
        vio.vi.clear();
        for (auto& object : visibleIndicesBuffersMapped) {
            memcpy(object, vio.vi.data(), vio.vi.size() * sizeof(uint32_t));
        }
    }
}

void Engine::updateDrawCommands(void* drawCommandsBufferMapped, DrawCommandsBufferObject& dc, ModelBus& mdlBus) {
    if (mdlBus.dirtyCommands) {
        dc.commands.clear();

        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
        uint32_t firstInstance = 0;

        for (const auto& group : std::views::values(mdlBus.groups_map)) {
            const auto& model = group.model;
            VkDrawIndexedIndirectCommand command{};

            const size_t instanceCount = group.instances.size();
            const uint32_t indexCount = ModelBus::getIndexCount(group.model);

            static bool initialized = false;
            if (!initialized) {
                command.firstInstance = firstInstance;
                command.instanceCount = group.instances.size();
            }

            command.firstIndex = firstIndex;
            command.indexCount = indexCount;
            command.vertexOffset = vertexOffset;

            firstIndex += indexCount;
            vertexOffset += ModelBus::getVertexCount(group.model);
            firstInstance += instanceCount;
            dc.commands.emplace_back(command);
        }

        mdlBus.dirtyCommands = true;
        static bool initialized = true;


        memcpy(drawCommandsBufferMapped, dc.commands.data(), dc.commands.size() * sizeof(VkDrawIndexedIndirectCommand));
    }
}
#pragma endregion


#pragma region Initialization
// Initialization of engine and its counterparts
void Engine::initialize(SDL_Window* window) {
    const auto start = std::chrono::high_resolution_clock::now();

    this->window = window;

    mdlBus.test();

    // Engine related stuff
    sid = Random::randomNum<uint64_t>(1000000000,9999999999);
    LOGGER = Logger::of("engine.hpp");

    initializeVulkan();
    initializeImGui(window);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Success!?
    LOGGER->success("2g43s engine started successfully in ${} seconds!", duration.count());
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
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Instance info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Vulkan extensions
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layers initialization
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        VkValidationFeaturesEXT validationFeatures = {};
        VkValidationFeatureEnableEXT enables[] = {};
        validationFeatures.pEnabledValidationFeatures = enables;
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

void Engine::initializeImGui(SDL_Window* window) {
    ImGui::CreateContext();

    ImGui_ImplSDL3_InitForVulkan(window);

    QueueFamilyIndices indices = Queue::findQueueFamilies(physicalDevice, surface);

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

    VkCommandBuffer commandBuffer = Command::beginSingleTimeCommands(device, graphicsCommandPool);
    Command::endSingleTimeCommands(device, commandBuffer, graphicsCommandPool, graphicsQueue);

    vkDeviceWaitIdle(device);
}

void Engine::initializeVulkan() {
    deltaT = new Delta();

    // Basic things
    createInstance();
    setupDebugMessenger();

    // SDL3 surface
    Surface::createSurface(Engine::surface, window, instance);

    // Devices
    PhysDevice::pickPhysicalDevice(physicalDevice, instance, surface);
    LogDevice::createLogicalDevice(device, physicalDevice, graphicsQueue, presentQueue, surface);

    // Swapchain
    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    // Desccriptor
    Descriptor::createGraphicsDescriptorSetLayout(device, graphicsDescriptorSetLayout);
    Descriptor::createMatrixComputeDescriptorSetLayout(device, matrixComputeDescriptorSetLayout);
    Descriptor::createCullingComputeDescriptorSetLayout(device, cullingComputeDescriptorSetLayout);
    Descriptor::createPostprocessDescriptorSetLayout(device, postprocessDescriptorSetLayout);

    PipelineCreation::createGraphicsPipeline(device, physicalDevice, graphicsPipelineLayout, graphicsPipeline, graphicsDescriptorSetLayout, swapchainImageFormat);
    PipelineCreation::createMatrixComputePipeline(device, matrixComputeDescriptorSetLayout, matrixComputePipelineLayout, matrixComputePipeline);
    PipelineCreation::createCullingComputePipeline(device, cullingComputeDescriptorSetLayout, cullingComputePipelineLayout, cullingComputePipeline);

    Command::createCommandPool(device, physicalDevice, graphicsCommandPool, surface);

    Helper::createDepthResources(device, physicalDevice, depthImage, depthImageMemory, depthImageView, swapchainExtent);

    Images::createTextureImage(device, graphicsCommandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, textureImage, textureImageMemory);
    Images::createTextureImageView(device, textureImage, textureImageView);
    Images::createTextureSampler(device, physicalDevice, textureSampler);

    Buffers::createVertexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, mdlBus);
    Buffers::createIndexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, indexBuffer, indexBufferMemory, mdlBus);

    // Generic
    Buffers::createUniformBuffers(device, physicalDevice, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT);
    Buffers::createAtomicCounterBuffers(device, physicalDevice, atomicCounterBuffers, atomicCounterBuffersMemory, atomicCounterBuffersMapped, MAX_FRAMES_IN_FLIGHT);

    // Matrices
    Buffers::createModelBuffers(device, physicalDevice, modelBuffers, modelBuffersMemory, modelBuffersMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);
    Buffers::createModelDataBuffers(device, physicalDevice, modelDataBuffers, modelDataBuffersMemory, modelDataBuffersMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);

    // Culling
    Buffers::createUniformCullingBuffers(device, physicalDevice, uniformCullingBuffers, uniformCullingBuffersMemory, uniformCullingBuffersMapped, MAX_FRAMES_IN_FLIGHT);
    Buffers::createVisibleIndicesBuffers(device, physicalDevice, visibleIndicesBuffers, visibleIndicesMemory, visibleIndicesMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);

    Buffers::createModelCullingBuffer(device, physicalDevice, modelCullingBuffer, modelCullingBufferMemory, modelCullingBufferMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);
    Buffers::createDrawCommandsBuffer(device, physicalDevice, drawCommandsBuffer, drawCommandsBufferMemory, mdlBus, drawCommandsBufferMapped, dc);

    // Descriptor
    Descriptor::createGraphicsDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorPool);
    Descriptor::createGraphicsDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout, graphicsDescriptorPool, graphicsDescriptorSets, uniformBuffers, textureImageView, textureSampler, modelBuffers, visibleIndicesBuffers, mdlBus);

    Descriptor::createMatrixComputeDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, matrixComputeDescriptorPool);
    Descriptor::createMatrixComputeDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, matrixComputeDescriptorSetLayout, matrixComputeDescriptorPool, matrixComputeDescriptorSets, modelBuffers, mdlBus, modelDataBuffers);

    Descriptor::createCullingComputeDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, cullingComputeDescriptorPool);
    Descriptor::createCullingComputeDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, cullingComputeDescriptorSetLayout, cullingComputeDescriptorPool, cullingComputeDescriptorSets, mdlBus, drawCommandsBuffer, uniformCullingBuffers, visibleIndicesBuffers, modelCullingBuffer);

    Descriptor::createPostprocessDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, postprocessDescriptorPool);
    Descriptor::createPostprocessComputeDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, postprocessDescriptorSetLayout, postprocessDescriptorPool, postprocessDescriptorSets, uniformCullingBuffers, textureImageView, textureSampler);

    Buffers::createCommandBuffer(device, graphicsCommandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT);
    Helper::createSyncObjects(device, imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences, MAX_FRAMES_IN_FLIGHT);
}
#pragma endregion


#pragma region Swapchain
void Engine::cleanupSwapchain() const {
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);

    for (auto & swapchainImageView : swapchainImageViews) {
        vkDestroyImageView(device, swapchainImageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Engine::recreateSwapchain() {
    vkDeviceWaitIdle(device);

    cleanupSwapchain();

    // Swapchain
    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    Helper::createDepthResources(device, physicalDevice, depthImage, depthImageMemory, depthImageView, swapchainExtent);
}
#pragma endregion


#pragma region Helper
// Setup debug messenger
void Engine::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    Debug::populateDebugMessengerCreateInfo(createInfo);

    if (Debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

// Get SDL extensions
std::vector<const char*> Engine::getRequiredExtensions() {
        uint32_t extensionCount = 0;
        auto SDLextensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

        std::vector extensions(SDLextensions, SDLextensions + extensionCount);

        // If validation layers enabled when add them to extensions vector
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        //extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        return extensions;
    }
#pragma endregion


#endif //ENGINE_CPP
