#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include <map>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

#include "../window/Swapchain.hpp"
#include "../util/Queue.hpp"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


struct PhysDevice {
    // Main method for picking Graphics device
    static void pickPhysicalDevice(VkPhysicalDevice& physicalDevice, VkInstance& instance, VkSurfaceKHR& surface) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, VkPhysicalDevice> candidates;

        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device, surface);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // Rating devices (for multi gpu setups)
    static int rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface) {
        Logger* LOGGER = Logger::of("physDevice.hpp");

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceFeatures supportedFeatures;

        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        int score = 0;

        if (!supportedFeatures.samplerAnisotropy) {
            score -= 1000;
            LOGGER->warn("Your GPU does not support sampler anisotropy!");
        }

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders
        if (!deviceFeatures.geometryShader) {
            return 0;
        }

        bool extensionsSupport = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupport) {
            SwapchainSupportDetails swapChainSupport = Swapchain::querySwapchainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        QueueFamilyIndices indices = Queue::findQueueFamilies(device, surface);

        if (!indices.isComplete() && !extensionsSupport && !swapChainAdequate) return 0;

        return score;
    }

    static bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }
};

#endif //PHYSICALDEVICE_H
