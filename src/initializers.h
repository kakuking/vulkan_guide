#pragma once

#include "utils.h"

class Initializers{
public:
    static VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags){
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.queueFamilyIndex = queueFamilyIndex;
        info.flags = flags;
        return info;
    }

    static VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count)
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.pNext = nullptr;

        info.commandPool = pool;
        info.commandBufferCount = count;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return info;
    }
};