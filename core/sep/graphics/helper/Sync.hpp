//
// Created by down1 on 29.01.2026.
//

#ifndef INC_2G43S_SYNC_H
#define INC_2G43S_SYNC_H
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <boost/hana.hpp>

struct Sync {
    template<std::convertible_to<std::vector<VkSemaphore>>... Semaphores>
static void createSemaphores(VkDevice& device, const size_t MAX_FRAMES_IN_FLIGHT, Semaphores&... pack) {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        ([&](auto& semaphores) {
            semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            for (auto& semaphore : semaphores) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore)!= VK_SUCCESS) {
                    throw std::runtime_error("failed to create semaphore!");
                }
            }
        }(pack), ...);
    }

    template<std::convertible_to<std::vector<VkFence>>... Fences>
    static void createFences(VkDevice& device, const size_t MAX_FRAMES_IN_FLIGHT, Fences&... pack) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        ([&](auto& fences) {
            fences.resize(MAX_FRAMES_IN_FLIGHT);
            for (auto& fence : fences) {
                if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create fence!");
                }
            }
        }(pack), ...);
    }
};

#endif //INC_2G43S_SYNC_H