//
// Created by down1 on 25.02.2026.
//

#include "SwapchainManager.hpp"

#include "Descriptor.hpp"
#include "Images.hpp"

void SwapchainManager::cleanupOffscreenImages() const {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyImageView(device, offscreenImageViews[i], nullptr);
        vkDestroyImage(device, offscreenImages[i], nullptr);
        vkFreeMemory(device, offscreenImagesMemory[i], nullptr);
    }
}

void SwapchainManager::cleanupSwapchain() const {
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
    vkDestroyImageView(device, depthImageView, nullptr);

    for (auto& swapchainImageView : swapchainImageViews) {
        vkDestroyImageView(device, swapchainImageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void SwapchainManager::initializeOffscreenImages(const uint32_t width, const uint32_t height) {
    offscreenImages.resize(MAX_FRAMES_IN_FLIGHT);
    offscreenImageViews.resize(MAX_FRAMES_IN_FLIGHT);
    offscreenImagesMemory.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        Images::createImage(device, physicalDevice, width, height, VK_FORMAT_R16G16B16A16_SFLOAT , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenImages[i], offscreenImagesMemory[i]);
        offscreenImageViews[i] = Images::createImageView(device, offscreenImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapchainManager::recreateSwapchain(const std::vector<VkDescriptorSet>& postprocessDescriptorSets) {
    vkDeviceWaitIdle(device);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    // Cleanup
    cleanupOffscreenImages();
    cleanupSwapchain();

    initializeOffscreenImages(width, height);

    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    Helper::createDepthResources(device, physicalDevice, depthImage, depthImageMemory, depthImageView, swapchainExtent);

    Descriptor::updatePostprocessDescriptorSets(device, postprocessDescriptorSets, offscreenImageViews, depthImageView, textureSampler, MAX_FRAMES_IN_FLIGHT);
}

void SwapchainManager::initializeSwapchain() {
    Swapchain::createSwapchain(device, physicalDevice, surface, window, swapchain, swapchainImages, swapchainImageFormat, swapchainExtent);
    Swapchain::createImageViews(device, swapchainImageViews, swapchainImages, swapchainImageFormat);

    // Offscreen images used for postprocessing
    initializeOffscreenImages(swapchainExtent.width, swapchainExtent.height);
}