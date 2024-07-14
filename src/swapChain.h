#pragma once
#include "utils.h"

#include "queueFamilies.h"
#include "image.h"
#include "swapChainSupportDetails.h"
#include "depthBuffer.h"

class SwapChain{
    public:
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFrameBuffers;

        SwapChain(){}

        void setupSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, GLFWwindow* window){
            createSwapChain(physicalDevice, surface, device, window);
        }

        void setupSwapChainImageViews(VkDevice device){
            createImageViews(device);
        }

        void setupFrameBuffer(VkDevice device, VkImageView depthImageView, VkRenderPass renderPass){
            createFrameBuffer(device, depthImageView, renderPass);
        }

        void recreateSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, GLFWwindow* window, DepthBuffer depthBuffer, VkRenderPass renderPass){
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            while(width == 0 || height == 0){
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);

            cleanupSwapChain(device, depthBuffer);

            createSwapChain(physicalDevice, surface, device, window);
            createImageViews(device);
            depthBuffer.setupDepthResources(device, physicalDevice, swapChainExtent);
            createFrameBuffer(device, depthBuffer.depthImageView, renderPass);
        }

        void cleanupSwapChain(VkDevice device, DepthBuffer& depthBuffer){
            vkDestroyImageView(device, depthBuffer.depthImageView, nullptr);
            vkDestroyImage(device, depthBuffer.depthImage, nullptr);
            vkFreeMemory(device, depthBuffer.depthImageMemory, nullptr);

            for(size_t i = 0; i < swapChainFrameBuffers.size(); i++){
                vkDestroyFramebuffer(device, swapChainFrameBuffers[i], nullptr);
            }

            for(size_t i = 0; i < swapChainImageViews.size(); i++){
                vkDestroyImageView(device, swapChainImageViews[i], nullptr);
            }

            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }
    
    private:
        void createSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, GLFWwindow* window){
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

            if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            if (indices.graphicsFamily != indices.presentFamily){
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0;
                createInfo.pQueueFamilyIndices = nullptr;
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }

            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
            swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

            swapChainImageFormat = surfaceFormat.format;
            swapChainExtent = extent;
        }

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
            for (const auto& availableFormat: availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                ) {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode: availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window){
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            } 

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }

        void createImageViews(VkDevice device){
            swapChainImageViews.resize(swapChainImages.size());

            for (size_t i = 0; i < swapChainImages.size(); i++){
                swapChainImageViews[i] = Image::createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);
            }
        }

        void createFrameBuffer(VkDevice device, VkImageView depthImageView, VkRenderPass renderPass){
            swapChainFrameBuffers.resize(swapChainImageViews.size());

            for(size_t i = 0; i < swapChainImageViews.size(); i++){
                std::array<VkImageView, 2> attachments = {
                    swapChainImageViews[i],
                    depthImageView
                };

                VkFramebufferCreateInfo frameBufferInfo{};
                frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                frameBufferInfo.renderPass = renderPass;
                frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
                frameBufferInfo.pAttachments = attachments.data();
                frameBufferInfo.width = swapChainExtent.width;
                frameBufferInfo.height = swapChainExtent.height;
                frameBufferInfo.layers = 1;

                if (vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }
};