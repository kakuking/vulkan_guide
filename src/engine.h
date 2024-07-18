#pragma once

#include "utils.h"

class Engine {
public:    
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;
    VkFormat swapchainImageFormat;

    Engine(){}

    void init(){
        setupWindow();
        setupVulkan();
        
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

    void draw(){
        
    }

    void cleanup(){
        destroySwapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
        cleanupWindow();
    }

private:
    double FPS;

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
        setupSwapchain();
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

    void cleanupWindow(){
        glfwDestroyWindow(window);

        glfwTerminate();
    }
};
