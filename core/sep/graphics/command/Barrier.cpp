//
// Created by down1 on 07.01.2026.
//

#include "Barrier.h"


Barrier& Barrier::image(
    const VkImage &image,
    const VkImageLayout &oldLayout, const VkImageLayout &newLayout,
    const VkAccessFlags2 &srcAccessMask, const VkAccessFlags2 &dstAccessMask,
    const VkPipelineStageFlags2 &srcStageMask, const VkPipelineStageFlags2 &dstStageMask,
    const VkImageAspectFlags &aspectMask
) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.image = image;

    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    barrier.srcStageMask = srcStageMask;
    barrier.dstStageMask = dstStageMask;

    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    imageBarriers.emplace_back(barrier);

    return *this;
}

Barrier& Barrier::buffer(
    const VkBuffer &buffer,
    const uint32_t &srcQueue, const uint32_t &dstQueue,
    const VkAccessFlags2 &srcAccessMask, const VkAccessFlags2 &dstAccessMask,
    const VkPipelineStageFlags2 &srcStageMask, const VkPipelineStageFlags2 &dstStageMask,
    const VkDeviceSize offset, const VkDeviceSize size
) {
    VkBufferMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.buffer = buffer;

    barrier.srcQueueFamilyIndex = srcQueue;
    barrier.dstQueueFamilyIndex = dstQueue;

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    barrier.srcStageMask = srcStageMask;
    barrier.dstStageMask = dstStageMask;

    barrier.offset = offset;
    barrier.size = size;

    bufferBarriers.emplace_back(barrier);

    return *this;
}


void Barrier::apply() const {
    VkDependencyInfo dependencyInfo{};

    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
    dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
    dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
    dependencyInfo.pMemoryBarriers = memoryBarriers.data();
    dependencyInfo.memoryBarrierCount = memoryBarriers.size();

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}
