//
// Created by down1 on 25.02.2026.
//

#ifndef INC_2G43S_SWAPCHAINMANAGER_H
#define INC_2G43S_SWAPCHAINMANAGER_H

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Swapchain.hpp"

struct SwapchainManager {
    explicit SwapchainManager() = default;

    SwapchainManager(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SDL_Window* window, VkSampler textureSampler, size_t MAX_FRAMES_IN_FLIGHT) :
    device(device), physicalDevice(physicalDevice),
    surface(surface), window(window),
    textureSampler(textureSampler),
    MAX_FRAMES_IN_FLIGHT(MAX_FRAMES_IN_FLIGHT) {}

    // Swapchain
    VkSwapchainKHR swapchain{};

    VkFormat swapchainImageFormat{};
    VkExtent2D swapchainExtent{};

    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};

    // Postprocess
    std::vector<VkImage> offscreenImages{};
    std::vector<VkDeviceMemory> offscreenImagesMemory{};
    std::vector<VkImageView> offscreenImageViews{};

    // Depth
    VkImage depthImage{};
    VkDeviceMemory depthImageMemory{};
    VkImageView depthImageView{};

    #pragma region Dependencies
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    SDL_Window* window;
    size_t MAX_FRAMES_IN_FLIGHT;
    VkSampler textureSampler;
    #pragma endregion

    void cleanupOffscreenImages() const;

    void cleanupSwapchain() const;

    void initializeOffscreenImages(uint32_t width,  uint32_t height);

    void recreateSwapchain(const std::vector<VkDescriptorSet> &postprocessDescriptorSets);

    void initializeSwapchain();
};


#endif //INC_2G43S_SWAPCHAINMANAGER_H