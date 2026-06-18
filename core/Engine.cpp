#ifndef ENGINE_CPP
#define ENGINE_CPP
#include <vulkan/vulkan_core.h>
#include "Engine.hpp"

#include "Random.hpp"


#include "Tools.hpp"
#include "Surface.hpp"

#include "PipelineCreation.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "GraphicsManager.hpp"
#include "imgui_impl_vulkan.h"
#include "PhysicsBus.hpp"
#include "Shaders.hpp"

#pragma region Main
// Initialization of engine and its counterparts
void Engine::initialize(SDL_Window* sdl_window) {
    const auto start = std::chrono::high_resolution_clock::now();

    const std::vector<std::filesystem::path> dirtyShaders = Shaders::getShadersToCompile();
    for (auto& dirtyShader : dirtyShaders) {
        auto shader = Shaders::compileShader(dirtyShader);
        Shaders::saveShaderToFile(Tools::getShaderPath() + "compiled/" + dirtyShader.lexically_relative(Tools::getShaderPath()).replace_extension(".spv").string(), shader);
    }

    this->window = sdl_window;

    // Engine related stuff
    sid = Random::randomNum<uint64_t>(1000000000,9999999999);
    logger = Logger("engine.hpp");

    initializeCore();
    initializeVulkan();

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;

    // Success!?
    logger.success("2g43s engine STARTED successfully in ${} seconds!", duration.count());
    initialized = true;
}

// Clean trash before closing app
void Engine::cleanup() const {
    vkDeviceWaitIdle(device);

    swapchainManager.cleanupSwapchain();
    swapchainManager.cleanupOffscreenImages();

    graphicsManager.cleanupTextures();

    bufferManager.cleanup();

    ImGui_ImplVulkan_Shutdown();

    graphicsManager.cleanup();

    for (size_t i = 0; i < framesInFlight; i++) {
        vkDestroyBuffer(device, bufferManager.drawCommandsBuffers[i], nullptr);
        vkFreeMemory(device, bufferManager.drawCommandsBuffersMemory[i], nullptr);

        vkDestroyBuffer(device, bufferManager.drawCommandsSourceBuffers[i], nullptr);
        vkFreeMemory(device, bufferManager.drawCommandsSourceBuffersMemory[i], nullptr);
    }

    vkDestroyDevice(device, nullptr);

    if constexpr (enableValidationLayers) {
        Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
#pragma endregion


#pragma region Initialization
void Engine::initializeCore() {
    delta = DeltaManager();

    // Basic things
    createVulkanInstance();
    setupDebugMessenger();
    modelEntityManager.scene();

    // SDL3 surface
    Surface::createSurface(surface, window, instance);
}

#pragma region Vulkan
// Creating Vulkan instance
void Engine::createVulkanInstance() {
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

    // Trying to create vulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void Engine::initializeDevices() {
    PhysicalDevice::pickPhysicalDevice(physicalDevice, instance, surface);
    PhysicalDevice::getSubgroupProperties(physicalDevice, subgroupProperties);
    LogicalDevice::createLogicalDevice(device, physicalDevice, graphicsQueue, presentQueue, surface);

    logger.info("Subgroup size: ${}", subgroupProperties.subgroupSize);
}

void Engine::initializeVulkan() {
    initializeDevices();

    bufferManager = BufferManager(device, physicalDevice, &modelEntityManager, &swapchainManager, &camera, framesInFlight);

    swapchainManager = SwapchainManager(device, physicalDevice, surface, window, textureSampler, framesInFlight);
    swapchainManager.initializeSwapchain();

    graphicsManager = GraphicsManager(device, physicalDevice, &bufferManager, &swapchainManager, &modelEntityManager, &delta, presentQueue, graphicsQueue, instance, &framebufferResized, framesInFlight, &camera, desiredFrameRate, sleepTimeTotalNS);
    graphicsManager.initialize();
}

#pragma endregion
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
