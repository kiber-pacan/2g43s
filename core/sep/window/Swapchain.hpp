//
// Created by daunita on 7/11/24.
//

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <vector>
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

// Details about support of certain features by machine
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
};

struct Swapchain {

    // Main method for creating swapchain
    static void createSwapchain(const VkDevice& device, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, SDL_Window* window, VkSwapchainKHR& swapchain, std::vector<VkImage>& swapchainImages, VkFormat& swapchainImageFormat, VkExtent2D& swapchainExtent);

    static SwapchainSupportDetails querySwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

    static void createImageViews(const VkDevice& device, std::vector<VkImageView>& swapchainImageViews, const std::vector<VkImage>& swapchainImages, const VkFormat& swapchainImageFormat);


    // Choosing swap chain color format
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    // Choosing present swap chain mode (Vsync)
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    // Choosing swap chain extent (width and height)
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
};



#endif //SWAPCHAIN_H
