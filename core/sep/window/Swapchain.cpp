//
// Created by daunita on 7/11/24.
//

#ifndef SWAPCHAIN_CPP
#define SWAPCHAIN_CPP

#include "Swapchain.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

#include "Images.hpp"
#include "Queue.hpp"



// Main method for creating swapchain
void Swapchain::createSwapchain(const VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, SDL_Window* window, VkSwapchainKHR& swapchain, std::vector<VkImage>& swapchainImages, VkFormat& swapchainImageFormat, VkExtent2D& swapchainExtent) {
    const SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice, surface);

    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);

    // Width / Height
    const VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = Queue::findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}

SwapchainSupportDetails Swapchain::querySwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
    SwapchainSupportDetails details;

    // Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // Just some little precaution
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    // Another little precaution
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void Swapchain::createImageViews(const VkDevice& device, std::vector<VkImageView>& swapchainImageViews, const std::vector<VkImage>& swapchainImages, const VkFormat& swapchainImageFormat) {
    swapchainImageViews.resize(swapchainImages.size());

    for (uint32_t i = 0; i < swapchainImages.size(); i++) {
        swapchainImageViews[i] = Images::createImageView(device, swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}


// Choosing swap chain color format
VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// Choosing present swap chain mode (VSync)
VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// Choosing swap chain extent (width and height)
VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        //max, std::max(min, v)
        actualExtent.width = std::min(capabilities.maxImageExtent.width, std::max(capabilities.minImageExtent.width, actualExtent.width));
        actualExtent.height = std::min(capabilities.maxImageExtent.height, std::max(capabilities.minImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}



#endif //SWAPCHAIN_CPP
