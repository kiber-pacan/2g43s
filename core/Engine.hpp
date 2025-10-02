#ifndef ENGINE_H
#define ENGINE_H

#include <SDL3/SDL_vulkan.h>

#include "Tools.hpp"
#include "Logger.hpp"
#include "sep/util/Debug.hpp"
#include "sep/device/LogDevice.hpp"
#include "sep/device/PhysDevice.hpp"
#include "sep/window/Surface.hpp"
#include "sep/window/Swapchain.hpp"
#include "sep/graphics/Graphics.hpp"
#include "glm/glm.hpp"
#include "sep/camera/Camera.h"
#include "sep/graphics/Delta.hpp"
#include "sep/model/ModelInstance.h"
#include "sep/model/ParsedModel.h"

// Parameters
inline int HEIGHT = 720;
inline int WIDTH = 1280;

class Engine {
public:
    // Method for initializing and running engine
    #pragma region Public

    void init(SDL_Window* window) {
        initialize(window);
    }

    static void updateUniformBuffer(uint32_t currentImage, VkExtent2D& swapchainExtent, std::vector<void*>& uniformBuffersMapped, Graphics::UniformBufferObject& ubo, glm::vec3 pos, Camera& camera) {
        ubo.model = glm::translate(glm::mat4(1.0f), pos);
        ubo.view = glm::lookAt(camera.pos + glm::vec3(0), camera.pos + camera.look, glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(camera.fov,  (float) swapchainExtent.width / (float) swapchainExtent.height, 0.1f, 2048.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

        updateUniformBuffer(currentFrame, swapchainExtent, uniformBuffersMapped, ubo, glm::vec3(), camera);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        Graphics::recordCommandBuffer(commandBuffers[currentFrame], imageIndex, renderPass, swapChainFramebuffers, swapchainExtent, graphicsPipeline, indices, vertexBuffer, indexBuffer, pipelineLayout, descriptorSets, currentFrame, clear_color);

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
    void cleanup() {
        vkDeviceWaitIdle(device);
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);

        cleanupSwapchain();

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

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
            Debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    #pragma endregion

    #pragma region PublicVars
    // Session ID
    uint64_t sid{};

    // Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    bool framebufferResized = false;
    Graphics::UniformBufferObject ubo{};

    // Controls
    Camera camera{};

    Delta *deltaT{};

    Color clear_color = Color::hex(0x80c4b5);
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
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // Shader stuff
    std::vector<Graphics::Vertex> vertices;

    std::vector<uint32_t> indices;

    #pragma endregion

    std::vector<ParsedModel> mdls_p;
    std::vector<ModelInstance> mdls_i;

    void loadModels() {
        std::string location = "/mnt/sda1/CLionProjects/2g43s/core/models/";
        std::vector<std::string> names;

        for (const std::string& name : names) {
            std::filesystem::path path = location + name;
            mdls_p.emplace_back(ParsedModel(path));
        }
    }

    // Initializaiton of engine and its counterparts
    void initialize(SDL_Window* window) {

        this->window = window;
        std::filesystem::path path = "/mnt/sda1/CLionProjects/2g43s/core/models/cabinet.glb";
        ParsedModel model = ParsedModel(path);


        for (std::vector<uint32_t> index : model.indices) {
            indices.append_range(index);
        }

        for (std::vector<Graphics::Vertex> mesh : model.meshes) {
            vertices.append_range(mesh);
        }

        // Engine related stuff
        sid = Tools::randomNum<uint64_t>(1000000000,9999999999);
        LOGGER = Logger::of("engine.hpp");

        initializeVulkan();

        // Success!?
        LOGGER->success("Vulkan engine started successfully!");
    }

    // Vulkan related stuff
    void initializeVulkan() {
        ubo.model = glm::mat4(1.0f);
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
        Graphics::createDescriptorSetLayout(device, descriptorSetLayout);
        Graphics::createGraphicsPipeline(device, pipelineLayout, renderPass, graphicsPipeline, descriptorSetLayout, pipelineLayout, renderPass);
        Graphics::createCommandPool(device, physicalDevice, commandPool, surface);
        Graphics::createDepthResources(device, physicalDevice, commandPool, graphicsQueue, depthImage, depthImageMemory, depthImageView, swapchainExtent);
        Graphics::createFramebuffers(device, depthImageView, swapChainFramebuffers, swapchainImageViews, renderPass, swapchainExtent);
        Graphics::createTextureImage(device, commandPool, graphicsQueue, physicalDevice, stagingBuffer, stagingBufferMemory, textureImage, textureImageMemory);
        Graphics::createTextureImageView(device, textureImage, textureImageView);
        Graphics::createTextureSampler(device, physicalDevice, textureSampler);
        Graphics::createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue, vertexBuffer, vertexBufferMemory, vertices);
        Graphics::createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue, indexBuffer, indexBufferMemory, indices);
        Graphics::createUniformBuffers(device, physicalDevice, uniformBuffers, uniformBuffersMemory, uniformBuffersMapped, MAX_FRAMES_IN_FLIGHT);
        Graphics::createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT, descriptorPool);
        Graphics::createDescriptorSets(device, MAX_FRAMES_IN_FLIGHT, descriptorSetLayout, descriptorPool, descriptorSets, uniformBuffers, textureImageView, textureSampler);
        Graphics::createCommandBuffer(device, commandPool, commandBuffers, MAX_FRAMES_IN_FLIGHT);
        Graphics::createSyncObjects(device, imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences, MAX_FRAMES_IN_FLIGHT);
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
        Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
        Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

        Graphics::createDepthResources(device, physicalDevice, commandPool, graphicsQueue, depthImage, depthImageMemory, depthImageView, swapchainExtent);
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
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            Debug::populateDebugMessengerCreateInfo(debugCreateInfo);
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

#endif //ENGINE_H
