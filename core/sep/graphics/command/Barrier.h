//
// Created by down1 on 07.01.2026.
//

#ifndef INC_2G43S_BARRIER_H
#define INC_2G43S_BARRIER_H
#include <vector>

#include "graphics/helper/Images.hpp"




class Barrier {
    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers;
    std::vector<VkMemoryBarrier2> memoryBarriers;
    VkCommandBuffer& commandBuffer;

public:
    explicit Barrier(VkCommandBuffer& commandBuffer) : commandBuffer(commandBuffer) {
        imageBarriers.reserve(4);
        bufferBarriers.reserve(4);
        memoryBarriers.reserve(4);
    }

    Barrier& image(
        const VkImage &image,
        const VkImageLayout &oldLayout, const VkImageLayout &newLayout,
        const VkAccessFlags2 &srcAccessMask, const VkAccessFlags2 &dstAccessMask,
        const VkPipelineStageFlags2 &srcStageMask, const VkPipelineStageFlags2 &dstStageMask,
        const VkImageAspectFlags &aspectMask
    );

    Barrier& buffer(
    const VkBuffer &buffer,
    const uint32_t &srcQueue, const uint32_t &dstQueue,
    const VkAccessFlags2 &srcAccessMask, const VkAccessFlags2 &dstAccessMask,
    const VkPipelineStageFlags2 &srcStageMask, const VkPipelineStageFlags2 &dstStageMask,
    VkDeviceSize offset, VkDeviceSize size
    );

    Barrier& memory(
    const VkImage &image,
    const VkImageLayout &oldLayout, const VkImageLayout &newLayout,
    const VkAccessFlags2 &srcAccessMask, const VkAccessFlags2 &dstAccessMask,
    const VkPipelineStageFlags2 &srcStageMask, const VkPipelineStageFlags2 &dstStageMask,
    const VkImageAspectFlags &aspectMask
    );

    void apply() const;
};


#endif //INC_2G43S_BARRIER_H