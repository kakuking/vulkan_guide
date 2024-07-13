#include "utils.h"

#include "queueFamilies.cpp"

class CommandBuffer{
    public:
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        CommandBuffer(){}

        void setupCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface){
            createCommandPool(device, physicalDevice, surface);
        }
    
    private:
        void createCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface){
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

            if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS){
                throw std::runtime_error("failed to create command pool!");
            }
        }
};