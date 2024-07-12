#ifndef ENGINE_H
#define ENGINE_H


#include <SDL3/SDL_vulkan.h>

#include "tools.hpp"
#include "logger.hpp"
#include "sep/debug.hpp"
#include "sep/logDevice.hpp"
#include "sep/physDevice.hpp"
#include "sep/surface.hpp"



//Parameters
inline int HEIGHT = 360;
inline int WIDTH = 640;

class engine {
public:
    //Method for estanblishing and running engine
    void init(SDL_Window* window) {
        initialize(window);
    }

    //Clean trash before closing app
    void cleanup() {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
        SDL_Vulkan_DestroySurface(instance, surface, nullptr); //Maybe it is useless...
        vkDestroySurfaceKHR(instance, surface, nullptr);

        if (enableValidationLayers) {
            debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }

    //Session ID
    uint64_t sid{};
private:
    //Instance
    VkInstance instance{};

    //Debug
    VkDebugUtilsMessengerEXT debugMessenger{};

    //Logger
    logger* LOGGER{};

    //Devices
    VkPhysicalDevice physicalDevice{};
    VkDevice device{};

    //Window
    VkSurfaceKHR surface{};
    VkSwapchainKHR swapchain{};
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat{};
    VkExtent2D swapChainExtent{};
    SDL_Window* window{};

    //Queues
    VkQueue presentQueue{};
    VkQueue graphicsQueue{};

    //Initializaiton of engine and its counterparts
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
        createInstance();
        setupDebugMessenger();
        surface::createSurface(surface, window, instance);
        physDevice::pickPhysicalDevice(instance, physicalDevice, surface);
        logDevice::createLogicalDevice(physicalDevice, device, graphicsQueue, surface);
        std::pair<VkFormat, VkExtent2D> pair = swapchain::createSwapChain(physicalDevice, device, surface, window, swapchain, swapchainImages);
        swapchainImageFormat = pair.first;
        swapChainExtent = pair.second;
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

    // Main engine loop
    void mainLoop() {

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
