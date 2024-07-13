#include "utils.h"
#include "image.cpp"

class DepthBuffer{
    public:
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        void setupDepthResources(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtent){
            createDepthResources(device, physicalDevice, swapChainExtent);
        }

        static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
            return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                physicalDevice
            );
        }
    
    private:
        void createDepthResources(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtent){
            VkFormat depthFormat = findDepthFormat(physicalDevice);

            Image::createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, physicalDevice, device);

            depthImageView = Image::createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, device);
        }

        static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice) {
            for(VkFormat format: candidates){
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                    return format;
                } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }

            throw std::runtime_error("failed to find a supported format");
        }
};