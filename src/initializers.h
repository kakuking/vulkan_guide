#pragma once

#include "utils.h"

namespace Initializers{
    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags){
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.queueFamilyIndex = queueFamilyIndex;
        info.flags = flags;
        return info;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count)
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext = nullptr;

        info.commandPool = pool;
        info.commandBufferCount = count;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return info;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags){
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;
        info.pInheritanceInfo = nullptr;
        info.flags = flags;

        return info;
    }

    VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0){
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;

        return info;
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0){
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = flags;

        return info;
    }

    VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent){
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;

        info.imageType = VK_IMAGE_TYPE_2D;

        info.format = format;
        info.extent = extent;
        info.extent.depth = 1;

        info.mipLevels = 1;
        info.arrayLayers = 1;

        info.samples = VK_SAMPLE_COUNT_1_BIT;   // For MSAA

        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usageFlags;

        return info;
    }

    VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags){
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;

        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.aspectMask = aspectFlags;

        return info;
    }

    VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask){
        VkImageSubresourceRange subImage{};

        subImage.aspectMask = aspectMask;
        subImage.baseMipLevel = 0;
        subImage.levelCount = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

        return subImage;
    }

    VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore){
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;

        submitInfo.semaphore = semaphore;
        submitInfo.stageMask = stageMask;

        submitInfo.deviceIndex = 0;
        submitInfo.value = 1;

        return submitInfo;
    }

    VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer command){
        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext = nullptr;

        info.commandBuffer = command;
        info.deviceMask = 0;

        return info;
    }

    VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* command, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo){
        VkSubmitInfo2 info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext = nullptr;

        info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = waitSemaphoreInfo;

        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signalSemaphoreInfo;

        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = command;

        return info;
    }

    VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout){
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.pNext = nullptr;

        colorAttachment.imageView = view;
        colorAttachment.imageLayout = layout;
        colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        if(clear){
            colorAttachment.clearValue = *clear;
        }

        return colorAttachment;
    }

    VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment)
    {
        VkRenderingInfo renderInfo {};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.pNext = nullptr;

        renderInfo.renderArea = VkRect2D { VkOffset2D { 0, 0 }, renderExtent };
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorAttachment;
        renderInfo.pDepthAttachment = depthAttachment;
        renderInfo.pStencilAttachment = nullptr;

        return renderInfo;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo()
    {
        VkPipelineLayoutCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;

        // empty defaults
        info.flags = 0;
        info.setLayoutCount = 0;
        info.pSetLayouts = nullptr;
        info.pushConstantRangeCount = 0;
        info.pPushConstantRanges = nullptr;
        return info;
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,VkShaderModule shaderModule, const char * entry)
    {
        VkPipelineShaderStageCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.pNext = nullptr;

        info.stage = stage;
        info.module = shaderModule;
        info.pName = entry;
        return info;
    }
};