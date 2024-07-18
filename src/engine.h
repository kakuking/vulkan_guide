#pragma once

#include "utils.h"

#include "initializers.h"
#include "images.h"

struct FrameData {
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;
};


class Engine {
public:    
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

    FrameData frames[FRAME_OVERLAP];
    uint32_t frameNumber;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;
    VkFormat swapchainImageFormat;

    Engine(){}

    void init(){
        setupWindow();
        setupVulkan();
        setupSwapchain();
        setupCommandResources();
        setupSyncStructures();
    }

    void run(){
        double totalFrameTime = 0.0;
        int frameCount = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        while(!glfwWindowShouldClose(window)){
            auto frameStartTime = std::chrono::high_resolution_clock::now();

            glfwPollEvents();
            draw();

            auto frameEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> frameDuration = frameEndTime - frameStartTime;

            totalFrameTime += frameDuration.count();
            frameCount++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;
        double averageFrameTime = totalFrameTime / frameCount;
        double fps = 1000.0 / averageFrameTime;

        fmt::println("Total elapsed time: {} seconds", elapsedSeconds.count());
        fmt::println("Total frames: {}", frameCount);
        fmt::println("Average frame time: {}ms", averageFrameTime);
        fmt::println("Average FPS: {}", fps);
    }

    void cleanup(){
        vkDeviceWaitIdle(device);

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

            vkDestroyFence(device, frames[i].renderFence, nullptr);
            vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);
        }
        

        destroySwapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
        cleanupWindow();
    }

private:
    double FPS;

    void draw(){
        VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, VK_TRUE, 1000000000)); // timeout of 1 second
        VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

        VkCommandBuffer command = getCurrentFrame().mainCommandBuffer;

        VK_CHECK(vkResetCommandBuffer(command, 0));
        VkCommandBufferBeginInfo beginInfo = Initializers::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));

        Utility::transitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearValue;
        float flash = std::abs(std::sin(frameNumber/120.f));
        clearValue = {{0.0f, 0.0f, flash, 1.0f}};

        VkImageSubresourceRange clearRange = Initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdClearColorImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

        Utility::transitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(command));

        VkCommandBufferSubmitInfo commandInfo = Initializers::commandBufferSubmitInfo(command);

        VkSemaphoreSubmitInfo waitInfo = Initializers::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = Initializers::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

        VkSubmitInfo2 submitInfo = Initializers::submitInfo(&commandInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submitInfo, getCurrentFrame().renderFence));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;

        presentInfo.pImageIndices = &swapchainImageIndex;
        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        frameNumber++;
    }

    void setupWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Zombifier", nullptr, nullptr);
    }

    void setupVulkan(){
        vkb::Instance vkb_instance = setupInstanceAndDebugMessenger();
        setupSurface();
        setupPhysicalDevice(vkb_instance);
    }

    vkb::Instance setupInstanceAndDebugMessenger(){
        vkb::InstanceBuilder builder;

        auto instance_return_option = builder.set_app_name("Zombified")
                                            .request_validation_layers(USE_VALIDATION_LAYERS)
                                            .use_default_debug_messenger()
                                            .require_api_version(1, 3, 0)
                                            .build();

        vkb::Instance vkb_instance = instance_return_option.value();

        instance = vkb_instance.instance;
        debugMessenger = vkb_instance.debug_messenger;

        return vkb_instance;
    }

    void setupSurface(){
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Window surface");
        }
    }

    void setupPhysicalDevice(vkb::Instance vkb_instance){
        VkPhysicalDeviceVulkan13Features features;
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features.dynamicRendering = VK_TRUE;
        features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features features12;
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features12.bufferDeviceAddress = VK_TRUE;
        features12.descriptorIndexing = VK_TRUE;

        vkb::PhysicalDeviceSelector selector{vkb_instance};
        vkb::PhysicalDevice vkb_physicalDevice = selector
                                                .set_minimum_version(1, 3)
                                                .set_required_features_13(features)
                                                .set_required_features_12(features12)
                                                .set_surface(surface)
                                                .select()
                                                .value();

        vkb::DeviceBuilder deviceBuilder{vkb_physicalDevice};

        vkb::Device vkb_device = deviceBuilder.build().value();

        device = vkb_device.device;
        physicalDevice = vkb_device.physical_device;

        graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    }

    void setupSwapchain(){
        vkb::SwapchainBuilder builder{physicalDevice, device, surface};

        swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain vkb_swapchain = builder
                                        .set_desired_format(VkSurfaceFormatKHR{.format = swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                        .set_desired_extent(WIDTH, HEIGHT)
                                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                        .build()
                                        .value();
        
        swapchainExtent = vkb_swapchain.extent;
        swapchain = vkb_swapchain.swapchain;
        swapchainImages = vkb_swapchain.get_images().value();
        swapchainImageViews = vkb_swapchain.get_image_views().value();
    }

    void destroySwapchain(){
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        for (size_t i = 0; i < swapchainImageViews.size(); i++)
        {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        }
        
    }

    FrameData& getCurrentFrame() {
        return frames[frameNumber % FRAME_OVERLAP];
    }

    void setupCommandResources(){
        VkCommandPoolCreateInfo createInfo = Initializers::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
;
        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(device, &createInfo, nullptr, &frames[i].commandPool));

            VkCommandBufferAllocateInfo allocInfo = Initializers::commandBufferAllocateInfo(frames[i].commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].mainCommandBuffer));
        }
        
    }

    void setupSyncStructures(){
        VkFenceCreateInfo fenceCreateInfo = Initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = Initializers::semaphoreCreateInfo();

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
        }
        
    }

    void cleanupWindow(){
        glfwDestroyWindow(window);

        glfwTerminate();
    }
};
