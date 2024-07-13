#include "utils.h"

#include "queueFamilies.cpp"

class LogicalDevice{
    public:
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        LogicalDevice(){}

        void setupLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface){
            createLogicalDevice(physicalDevice, surface);
        }

    private:
        void createLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueuQueueFamilies =  {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queuePriority = 1.0f;
            
            for (uint32_t queueFamily : uniqueuQueueFamilies){
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }
            


            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();

            createInfo.pEnabledFeatures = &deviceFeatures;

            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            if(enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device");
            }

            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &presentQueue);
        }
};