#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <SDL3/SDL_vulkan.h>

#include "sep/util/Tools.hpp"
#include "sep/util/Logger.hpp"
#include "sep/util/Debug.hpp"
#include "sep/device/LogDevice.hpp"
#include "sep/device/PhysDevice.hpp"
#include "sep/window/Surface.hpp"
#include "sep/window/Swapchain.hpp"
#include "sep/graphics/Graphics.hpp"
#include "glm/glm.hpp"
#include "sep/camera/Camera.hpp"
#include "sep/graphics/Delta.hpp"
#include "sep/model/bus/ModelBus.hpp"
#include "sep/model/ModelInstance.hpp"
#include "sep/model/ParsedModel.hpp"
#include <ranges>

// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class Engine {
public:
    // Method for initializing and running engine
    #pragma region Basic
    void init(SDL_Window* window) {
        initialize(window);
    }

    // Draw
    void drawFrame() {
        auto start = std::chrono::high_resolution_clock::now();

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        auto& fwf = *reinterpret_cast<ModelBufferObject*>(modelBuffersMapped[currentFrame]);

        uint32_t countValue = *(uint32_t*)atomicCounterBuffersMapped[currentFrame];
        // std::cout << "Count: " << countValue << std::endl;

        updateUniformBuffer(currentFrame, swapchainExtent, uniformBuffersMapped, ubo, glm::vec3(), camera);
        updateModelDataBuffer(currentFrame, modelDataBuffersMapped, mdlBus);
        //updateAtomicCounterBuffer(currentFrame, atomicCounterBuffersMapped, atomic_counter);
        updateModelBuffer(currentFrame, modelBuffersMapped, mbo);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        Graphics::recordCommandBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, commandBuffers[currentFrame], imageIndex, renderPass, swapChainFramebuffers, swapchainExtent, graphicsPipeline, matrixComputePipeline, vertexBuffer, indexBuffer, graphicsPipelineLayout, matrixComputePipelineLayout, graphicsDescriptorSets, matrixComputeDescriptorSets, currentFrame, clear_color, mdlBus, modelBuffers, modelDataBuffers, drawCommandsBuffer, drawCommandsBufferMemory, MAX_FRAMES_IN_FLIGHT, matrixDirty, atomicCounterBuffersMapped, atomicCounterBuffers);

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

        static bool i = true;
        if (i) {
            i = false;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;

            LOGGER->success("draw frame in ${} seconds!", duration.count());
        }
    }


    // Clean trash before closing app
    void cleanup() const {
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

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, modelCullingBuffers[i], nullptr);
            vkFreeMemory(device, modelCullingBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, graphicsDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);

        vkDestroyDescriptorPool(device, matrixComputeDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, matrixComputeDescriptorSetLayout, nullptr);

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


        vkDestroyRenderPass(device, renderPass, nullptr);

        vkDestroyCommandPool(device, graphicsCommandPool, nullptr);

        vkDestroyCommandPool(device, matrixComputeCommandPool, nullptr);


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
    static void updateUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapchainExtent, const std::vector<void*>& uniformBuffersMapped, UniformBufferObject& ubo, const glm::vec3 pos, const Camera& camera) {
        ubo.view = glm::lookAt(camera.pos + glm::vec3(0), camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(camera.fov,  static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height), 0.1f, 4096.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    static void updateAtomicCounterBuffer(const uint32_t currentFrame, const std::vector<void*>& atomicBuffersMapped, AtomicCounterObject& atomic_counter) {
        atomic_counter.counter = 0;
        memcpy(atomicBuffersMapped[currentFrame], &atomic_counter, sizeof(atomic_counter));
    }

    // Matrices
    static void updateModelDataBuffer(const uint32_t currentFrame, const std::vector<void*>& modelDataBuffersMapped, const ModelBus& mdlBus) {
        static bool dirtyMDBO = true;
        static size_t frame = 0;
        if (dirtyMDBO) {
            auto* dst = static_cast<glm::vec4*>(modelDataBuffersMapped[currentFrame]);
            size_t i = 0;
            for (const auto& group : std::views::transform(std::views::values(mdlBus.groups_map), &ModelGroup::instances)) {
                for (const auto& instance : group) {
                    i++;
                    dst[i * 3 + 0] = instance.pos;
                    dst[i * 3 + 1] = instance.rot;
                    dst[i * 3 + 2] = instance.scl;
                }
            }

            frame++;
            if (frame == MAX_FRAMES_IN_FLIGHT) dirtyMDBO = false;
        }
    }

    static void updateModelBuffer(const uint32_t currentFrame, const std::vector<void*>& modelBuffersMapped, ModelBufferObject& mbo) {
        mbo.mdls.clear();
        memcpy(modelBuffersMapped[currentFrame], mbo.mdls.data(), mbo.mdls.size() * sizeof(glm::mat4));
    }

    // Culling
    static void updateModelCullingBuffer(const uint32_t currentFrame, const std::vector<void*>& modelCullingBuffersMapped, const ModelBus& mdlBus) {

    }

    static void updateVisibleIndicesBuffer(const uint32_t currentFrame, const std::vector<void*>& visibleIndicesBuffersMapped, VisibleIndicesBufferObject& vio) {
        vio.vi.clear();
        memcpy(visibleIndicesBuffersMapped[currentFrame], vio.vi.data(), vio.vi.size() * sizeof(uint32_t));
    }
    #pragma endregion



    #pragma region PublicVars
    // Window
    SDL_Window* window{};

    // Session ID
    uint64_t sid{};


    Color clear_color = Color::hex(0x80c4b5);

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    bool framebufferResized = false;

    // Generic
    UniformBufferObject ubo{};
    AtomicCounterObject atomic_counter{};

    // Matrices
    ModelDataBufferObject mdbo{};
    ModelBufferObject mbo{};

    // Culling
    VisibleIndicesBufferObject vio{};
    ModelCullingBufferObject mcbo{};

    // Controls
    Camera camera{};

    Delta *deltaT{};

    ModelBus mdlBus{};
    #pragma endregion PublicVars

private:
    #pragma region Variables
    // Instance
    VkInstance instance{};

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    // Logger
    Logger* LOGGER{};

    // Window
    VkSurfaceKHR surface{};

    // Queues
    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    // Swapchain
    VkSwapchainKHR swapchain{};

    VkFormat swapchainImageFormat{};
    VkExtent2D swapchainExtent{};

    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};
    std::vector<VkFramebuffer> swapChainFramebuffers{};

    // Graphics
    VkPipeline graphicsPipeline{};
    VkPipelineLayout graphicsPipelineLayout{};
    VkDescriptorSetLayout graphicsDescriptorSetLayout{};

    std::vector<VkDescriptorSet> graphicsDescriptorSets{};
    VkDescriptorPool graphicsDescriptorPool{};
    VkCommandPool graphicsCommandPool{};

    // Compute
    VkPipeline matrixComputePipeline{};
    VkPipelineLayout matrixComputePipelineLayout{};
    VkDescriptorSetLayout matrixComputeDescriptorSetLayout{};
    bool matrixDirty = true;

    std::vector<VkDescriptorSet> matrixComputeDescriptorSets{};
    VkDescriptorPool matrixComputeDescriptorPool{};
    VkCommandPool matrixComputeCommandPool{};


    VkPipeline cullingComputePipeline{};
    VkPipelineLayout cullingComputePipelineLayout{};
    VkDescriptorSetLayout cullingComputeDescriptorSetLayout{};

    std::vector<VkDescriptorSet> cullingComputeDescriptorSets{};
    VkDescriptorPool cullingComputeDescriptorPool{};
    VkCommandPool cullingComputeCommandPool{};


    VkRenderPass renderPass{};

    // Depth
    VkImage depthImage{};
    VkDeviceMemory depthImageMemory{};
    VkImageView depthImageView{};

    // Buffers
    // Main
    std::vector<VkCommandBuffer> commandBuffers{};

    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};

    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    VkBuffer drawCommandsBuffer{};
    VkDeviceMemory drawCommandsBufferMemory{};

    // Generic
    std::vector<VkBuffer> uniformBuffers{};
    std::vector<VkDeviceMemory> uniformBuffersMemory{};
    std::vector<void*> uniformBuffersMapped{};

    std::vector<VkBuffer> atomicCounterBuffers{};
    std::vector<VkDeviceMemory> atomicCounterBuffersMemory{};
    std::vector<void*> atomicCounterBuffersMapped{};


    // Matrices
    std::vector<VkBuffer> modelBuffers{};
    std::vector<VkDeviceMemory> modelBuffersMemory{};
    std::vector<void*> modelBuffersMapped{};

    std::vector<VkBuffer> modelDataBuffers{};
    std::vector<VkDeviceMemory> modelDataBuffersMemory{};
    std::vector<void*> modelDataBuffersMapped{};


    // Culling
    std::vector<VkBuffer> visibleIndicesBuffers{};
    std::vector<VkDeviceMemory> visibleIndicesMemory{};
    std::vector<void*> visibleIndicesMapped{};

    std::vector<VkBuffer> modelCullingBuffers{};
    std::vector<VkDeviceMemory> modelCullingBuffersMemory{};
    std::vector<void*> modelCullingBuffersMapped{};


    // Image
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};

    VkSampler textureSampler{};

    // Fences
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};

    // Frames
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // Shader stuff

    #pragma endregion

    // Initialization of engine and its counterparts
    void initialize(SDL_Window* window) {
        const auto start = std::chrono::high_resolution_clock::now();

        this->window = window;

        mdlBus.test();

        // Engine related stuff
        sid = Random::randomNum<uint64_t>(1000000000,9999999999);
        LOGGER = Logger::of("engine.hpp");

        initializeVulkan();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        // Success!?
        LOGGER->success("2g43s engine started successfully in ${} seconds!", duration.count());
    }

    // Vulkan related stuff
    void initializeVulkan() {
        deltaT = new Delta();

        // Basic things
        createInstance();
        setupDebugMessenger();

        // SDL3 surface
        Surface::createSurface(surface, window, instance);

        // Devices
        PhysDevice::pickPhysicalDevice(physicalDevice, instance, surface);
        LogDevice::createLogicalDevice(device, physicalDevice, graphicsQueue, presentQueue, surface);

        // Swapchain
        Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
        Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        // Graphics
        Graphics::createRenderPass(device, physicalDevice, swapchainImageFormat, renderPass);

        Graphics::createGraphicsDescriptorSetLayout(device, graphicsDescriptorSetLayout);
        Graphics::createMatrixComputeDescriptorSetLayout(device, matrixComputeDescriptorSetLayout);

        Graphics::createGraphicsPipeline(device, graphicsPipelineLayout, renderPass, graphicsPipeline, graphicsDescriptorSetLayout);
        Graphics::createMatrixComputePipeline(device, matrixComputeDescriptorSetLayout, matrixComputePipelineLayout, matrixComputePipeline);

        Graphics::createCommandPool(device, physicalDevice, graphicsCommandPool, surface);

        Graphics::createDepthResources(device, physicalDevice, graphicsCommandPool, graphicsQueue, depthImage, depthImageMemory, depthImageView, swapchainExtent);
        Graphics::createFramebuffers(device, depthImageView, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);

        Graphics::createTextureImage(device, graphicsCommandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, textureImage, textureImageMemory);
        Graphics::createTextureImageView(device, textureImage, textureImageView);
        Graphics::createTextureSampler(device, physicalDevice, textureSampler);

        Graphics::createVertexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, mdlBus);
        Graphics::createIndexBuffer(device, physicalDevice, graphicsCommandPool, graphicsQueue, indexBuffer, indexBufferMemory, mdlBus);

        // Generic
        Graphics::createUniformBuffers(device, physicalDevice, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT);
        Graphics::createAtomicCounterBuffers(device, physicalDevice, atomicCounterBuffers, atomicCounterBuffersMemory, atomicCounterBuffersMapped, MAX_FRAMES_IN_FLIGHT);
        // Matrices
        Graphics::createModelBuffers(device, physicalDevice, modelBuffers, modelBuffersMemory, modelBuffersMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);
        Graphics::createModelDataBuffers(device, physicalDevice, modelDataBuffers, modelDataBuffersMemory, modelDataBuffersMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);
        // Culling
        Graphics::createVisibleIndicesBuffers(device, physicalDevice, visibleIndicesBuffers, visibleIndicesMemory, visibleIndicesMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);
        Graphics::createModelCullingBuffers(device, physicalDevice, modelCullingBuffers, modelCullingBuffersMemory, modelCullingBuffersMapped, MAX_FRAMES_IN_FLIGHT, mdlBus);

        Graphics::createGraphicsDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorPool);
        Graphics::createGraphicsDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, graphicsDescriptorSetLayout, graphicsDescriptorPool, graphicsDescriptorSets, uniformBuffers, textureImageView, textureSampler, modelBuffers, mdlBus);

        Graphics::createMatrixComputeDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, matrixComputeDescriptorPool);
        Graphics::createMatrixComputeDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, matrixComputeDescriptorSetLayout, matrixComputeDescriptorPool, matrixComputeDescriptorSets, modelBuffers, mdlBus, modelDataBuffers, uniformBuffers, atomicCounterBuffers);

        Graphics::createCommandBuffer(device, graphicsCommandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT);


        Graphics::createSyncObjects(device, imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences, MAX_FRAMES_IN_FLIGHT);
    }


    void cleanupSwapchain() const {
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        vkDestroyImageView(device, depthImageView, nullptr);

        for (auto & swapChainFramebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);
        }

        for (auto & swapchainImageView : swapchainImageViews) {
            vkDestroyImageView(device, swapchainImageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    void recreateSwapchain() {
        vkDeviceWaitIdle(device);

        cleanupSwapchain();

        // Swapchain
        Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
        Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        Graphics::createDepthResources(device, physicalDevice, graphicsCommandPool, graphicsQueue, depthImage, depthImageMemory, depthImageView, swapchainExtent);
        Graphics::createFramebuffers(device, depthImageView, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);
    }


    // Creating Vulkan instance
    void createInstance() {
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

        // Trying to create vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }


    // Setup debug messenger
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Debug::populateDebugMessengerCreateInfo(createInfo);

        if (Debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // Get SDL extensions
    static std::vector<const char*> getRequiredExtensions() {
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
};

#endif //ENGINE_HPP
