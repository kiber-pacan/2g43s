#ifndef ENGINE_H
#define ENGINE_H


#include <SDL3/SDL_vulkan.h>

#include "tools.hpp"
#include "logger.hpp"
#include "sep/debug.hpp"
#include "sep/logDevice.hpp"
#include "sep/physDevice.hpp"
#include "sep/surface.hpp"
#include "graphics.hpp"



// Parameters
inline int HEIGHT = 360;
inline int WIDTH = 640;

class engine {
public:
    // Method for estanblishing and running engine
    void init(SDL_Window* window) {
        initialize(window);
    }

    // Clean trash before closing app
    void cleanup() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapchainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    // Draw
    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        graphics::recordCommandBuffer(commandBuffers[currentFrame], imageIndex, renderPass, swapChainFramebuffers, swapchainExtent, graphicsPipeline);

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

        VkSwapchainKHR swapChains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }


    // Session ID
    uint64_t sid{};

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};
private:
    // Instance
    VkInstance instance{};

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    // Logger
    logger* LOGGER{};

    // Window
    VkSurfaceKHR surface{};
    SDL_Window* window{};

    // Queues
    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    // Swap chain
    VkSwapchainKHR swapchain{};

    VkFormat swapchainImageFormat{};
    VkExtent2D swapchainExtent{};

    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};
    std::vector<VkFramebuffer> swapChainFramebuffers{};


    // Graphics
    VkPipeline graphicsPipeline{};
    VkRenderPass renderPass{};
    VkPipelineLayout pipelineLayout{};
    VkCommandPool commandPool{};
    std::vector<VkCommandBuffer> commandBuffers{};

    // Fences
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};

    // Frames
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // Initializaiton of engine and its counterparts
    void initialize(SDL_Window* window) {
        this->window = window;

        // Engine related stuff
        sid = tools::randomNum<uint64_t>(1000000000,9999999999);
        LOGGER = logger::of("ENGINE");

        initializeVulkan();

        // Success!?
        LOGGER->log(logger::severity::SUCCESS, "Vulkan engine started successfully!", nullptr);
    }

    // Vulkan related stuff
    void initializeVulkan() {
        // Basic things
        createInstance();
        setupDebugMessenger();

        // SDL3 surface
        surface::createSurface(surface, window, instance);

        // Devices
        physDevice::pickPhysicalDevice(instance, physicalDevice, surface);
        logDevice::createLogicalDevice(physicalDevice, device, graphicsQueue, presentQueue, surface);

        // Swapchain
        std::pair<VkFormat, VkExtent2D> pair = swapchain::createSwapChain(physicalDevice, device, surface, window, swapchain, swapchainImages);
        swapchainImageFormat = pair.first;
        swapchainExtent = pair.second;
        swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        // Graphics
        graphics::createRenderPass(device, swapchainImageFormat, renderPass);
        graphics::createGraphicsPipeline(device, pipelineLayout, renderPass, graphicsPipeline);
        graphics::createFramebuffers(device, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);
        graphics::createCommandPool(device, physicalDevice, commandPool, surface);
        graphics::createCommandBuffer(commandPool, device, commandBuffers, MAX_FRAMES_IN_FLIGHT);
        graphics::createSyncObjects(device, imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences, MAX_FRAMES_IN_FLIGHT);
    }




    // Creating Vulkan instance
    void createInstance() {
        // Check if validation layers available when requested
        if (enableValidationLayers && !debug::checkValidationLayerSupport()) {
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
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            debug::populateDebugMessengerCreateInfo(debugCreateInfo);
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

    // Get SDL extensions
    static std::vector<const char*> getRequiredExtensions() {
        uint32_t extensionCount = 0;
        auto SDLextensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

        std::vector<const char*> extensions(SDLextensions, SDLextensions + extensionCount);

        // If validation layers enabled when add them to extensions vector
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        return extensions;
    }

    // Setup debug messenger
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        debug::populateDebugMessengerCreateInfo(createInfo);

        if (debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }


};

#endif //ENGINE_H
