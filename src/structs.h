#pragma once

#include "utils.h"
#include <span>

struct DeletionQueue{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        for(auto it = deletors.rbegin(); it != deletors.rend(); it++){
            (*it)();
        }
        deletors.clear();
    }
};

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;
    DeletionQueue deletionQueue;
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory imageMemory;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type){
        VkDescriptorSetLayoutBinding newBind{};
        newBind.binding = binding;
        newBind.descriptorCount = 1;
        newBind.descriptorType = type;

        bindings.push_back(newBind);
    }

    void clear(){
        bindings.clear();
    }

    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0){
        for (auto& b: bindings)
        {
            b.stageFlags |= shaderStages;
        }

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = pNext;

        info.pBindings = bindings.data();
        info.bindingCount = static_cast<uint32_t>(bindings.size());
        info.flags = flags;
        
        VkDescriptorSetLayout set;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

        return set;
    }
};

struct DescriptorAllocator {
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios){
        std::vector<VkDescriptorPoolSize> poolSizes;
        for(PoolSizeRatio ratio: poolRatios){
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = ratio.type,
                .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)
            });
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = 0;
        poolInfo.maxSets = maxSets;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
    } 

    void clearDescriptors(VkDevice device){
        vkResetDescriptorPool(device, pool, 0);
    }

    void destroyPool(VkDevice device){
        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout){
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet ds;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

        return ds;
    }
};