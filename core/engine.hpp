#ifndef ENGINE_H
#define ENGINE_H

#include <SDL3/SDL_vulkan.h>

#include "tools.hpp"
#include "logger.hpp"
#include "sep/util/debug.hpp"
#include "sep/device/logDevice.hpp"
#include "sep/device/physDevice.hpp"
#include "sep/window/surface.hpp"
#include "sep/window/swapchain.hpp"
#include "sep/graphics/graphics.hpp"
#include "glm/glm.hpp"
#include "sep/camera/cam.h"
#include "sep/graphics/delta.hpp"

// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class engine {
public:
    // Method for initializing and running engine
    void init(SDL_Window* window) {
        initialize(window);
    }

    // Draw
    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame, swapchainExtent, uniformBuffersMapped, ubo, camera.pos, camera);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        graphics::recordCommandBuffer(commandBuffers[currentFrame], imageIndex, renderPass, swapChainFramebuffers, swapchainExtent, graphicsPipeline, indices, vertexBuffer, indexBuffer, pipelineLayout, descriptorSets, currentFrame);

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


    static void updateUniformBuffer(uint32_t currentImage, VkExtent2D& swapchainExtent, std::vector<void*>& uniformBuffersMapped, graphics::UniformBufferObject& ubo, glm::vec3 pos, cam& cam) {
        ubo.model = glm::translate(glm::mat4(1.0f), pos);
        ubo.view = glm::lookAt(glm::vec3(0), cam.look, glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(cam.fov,  (float) swapchainExtent.width / (float) swapchainExtent.height, 0.01f, 64.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    // Clean trash before closing app
    void cleanup() {
        cleanupSwapchain();

        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

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
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        vkDestroyRenderPass(device, renderPass, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    // Session ID
    uint64_t sid{};

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    bool framebufferResized = false;
    graphics::UniformBufferObject ubo{};

    // Controls
    cam camera{};

    delta *deltaT{};

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

    // Swapchain
    VkSwapchainKHR swapchain{};

    VkFormat swapchainImageFormat{};
    VkExtent2D swapchainExtent{};

    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};
    std::vector<VkFramebuffer> swapChainFramebuffers{};

    // Graphics
    VkPipeline graphicsPipeline{};
    VkPipelineLayout pipelineLayout{};

    VkDescriptorSetLayout descriptorSetLayout{};
    std::vector<VkDescriptorSet> descriptorSets{};

    VkDescriptorPool descriptorPool{};
    VkCommandPool commandPool{};

    VkRenderPass renderPass{};

    // Depth
    VkImage depthImage{};
    VkDeviceMemory depthImageMemory{};
    VkImageView depthImageView{};

    // Buffers

    std::vector<VkCommandBuffer> commandBuffers{};

    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};

    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};

    // Fences
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};

    // Frames
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // Shader stuff
    std::vector<graphics::Vertex> vertices = {
        { {-8.f, -8.f, -8.f}, {1.0f, 0.0f, 0.0f} },
        { {-8.f, -8.f, 8.f}, {0.0f, 1.0f, 0.0f} },
        { {-8.f, 8.f, 8.f}, {0.0f, 0.0f, 1.0f} },
        { {-8.f, 8.f, -8.f}, {1.0f, 1.0f, 1.0f} },
        { {8.f, -8.f, -8.f}, {1.0f, 0.0f, 0.0f} },
        { {8.f, -8.f, 8.f}, {0.0f, 1.0f, 0.0f} },
        { {8.f, 8.f, 8.f}, {0.0f, 0.0f, 1.0f} },
        { {8.f, 8.f, -8.f}, {1.0f, 1.0f, 1.0f} }
    };

    std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        0, 3, 2, 2, 1, 0,

        4, 5, 6, 6, 7, 4,
        4, 7, 6, 6, 5, 4,

        1, 5, 2, 5, 6, 2,
        2, 6, 5, 2, 5, 1,

        3, 7, 6, 6, 2, 3,
        3, 2, 6, 6, 7, 3,

        0, 3, 4, 3, 7, 4,
        4, 7, 3, 4, 3, 0,

        0, 1, 5, 0, 5, 4,
        4, 5, 0, 5, 1, 0
    };

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
        ubo.model = glm::mat4(1.0f);
        deltaT = new delta();

        // Basic things
        createInstance();
        setupDebugMessenger();

        // SDL3 surface
        surface::createSurface(surface, window, instance);

        // Devices
        physDevice::pickPhysicalDevice(physicalDevice, instance, surface);
        logDevice::createLogicalDevice(device, physicalDevice, graphicsQueue, presentQueue, surface);

        // Swapchain
        swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
        swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        // Graphics
        graphics::createRenderPass(device, swapchainImageFormat, renderPass);
        graphics::createDescriptorSetLayout(device, descriptorSetLayout);
        graphics::createGraphicsPipeline(device, pipelineLayout, renderPass, graphicsPipeline, descriptorSetLayout, pipelineLayout, renderPass);
        graphics::createFramebuffers(device, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);
        graphics::createCommandPool(device, physicalDevice, commandPool, surface);
        graphics::createTextureImage(device, commandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, textureImage, textureImageMemory);
        graphics::createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, vertices);
        graphics::createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue, indexBuffer, indexBufferMemory, indices);
        graphics::createUniformBuffers(device, physicalDevice, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT);
        graphics::createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, descriptorPool);
        graphics::createDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, descriptorSetLayout, descriptorPool, descriptorSets, uniformBuffers);
        graphics::createCommandBuffer(device, commandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT);
        graphics::createSyncObjects(device, imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences, MAX_FRAMES_IN_FLIGHT);
    }


    void cleanupSwapchain() {
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
        swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
        swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        graphics::createFramebuffers(device, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);
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


    // Setup debug messenger
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        debug::populateDebugMessengerCreateInfo(createInfo);

        if (debug::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
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

#endif //ENGINE_H
