#ifndef ENGINE_H
#define ENGINE_H

#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

#include "tools.hpp"
#include "logger.cpp"
#include "sep/debug.hpp"
#include "sep/physDevice.h"

//Parameters
inline int HEIGHT = 360;
inline int WIDTH = 640;

class engine {
public:
    //Method for estanblishing and running engine
    void init() {
        initVulkan();
        mainLoop();
        cleanup();
    }

    //Session ID
    uint64_t sid;
private:
    //Variables
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    logger* LOGGER;
    //Rendering device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


    //Init engine and its counterparts
    void initVulkan() {
        sid = tools::randomNum<uint64_t>(1000000000,9999999999);
        LOGGER = logger::of("ENGINE");
        LOGGER->log("76ff00", "Vulkan engine started successfully!", nullptr);
        createInstance();
        setupDebugMessenger();
        physDevice::pickPhysicalDevice(instance,physicalDevice);
    }

    //Creating Vulkan instance
    void createInstance() {
        //Check if validation layers available when requested
        if (enableValidationLayers && !debug::checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        //App info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        //Instance info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        //Vulkan extensions
        auto extensions = debug::getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();


        //Validation layers initialization
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

        //Trying to create vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    //Main engine loop
    void mainLoop() {

    }

    //Clean trash before closing app
    void cleanup() {
        if (enableValidationLayers) {
            debug::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }

    //Setup debug messenger
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
