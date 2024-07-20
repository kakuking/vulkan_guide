#pragma once

#include "utils.h"

#include "initializers.h"
#include "images.h"
#include "structs.h"
#include "pipelines.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

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

    AllocatedImage drawImage;
    VkExtent2D drawExtent;

    DeletionQueue mainDeletionQueue;
    VmaAllocator allocator;

    DescriptorAllocator globalDescriptorAllocator;

    VkDescriptorSet drawImageDescriptors;
    VkDescriptorSetLayout drawImageDescriptorLayout;

    VkPipeline gradientPipeline;
    VkPipelineLayout gradientPipelineLayout;

    VkFence immediateFence;
    VkCommandBuffer immediateCommandBuffer;
    VkCommandPool immediateCommandPool;

    VkPipelineLayout trianglePipelineLayout;
    VkPipeline trianglePipeline;

    VkPipelineLayout meshPipelineLayout;
    VkPipeline meshPipeline;

    GPUMeshBuffers rectangle;

    std::vector<ComputeEffect> backgroundEffects;
    int currentBackgroundEffect{0};

    Engine(){}

    void init(){
        setupWindow();
        setupVulkan();
        setupSwapchain();
        setupCommandResources();
        setupSyncStructures();
        setupDescriptors();
        setupPipeline();
        setupDefaultRectangleData();
        setupImgui();
    }

    void run(){
        double totalFrameTime = 0.0;
        int frameCount = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        while(!glfwWindowShouldClose(window)){
            auto frameStartTime = std::chrono::high_resolution_clock::now();

            glfwPollEvents();

            ImGui_ImplGlfw_NewFrame();
            ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();

            if(ImGui::Begin("Background")) {
                ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

                ImGui::Text("Selected Effect: ", selected.name);

                ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, backgroundEffects.size() - 1);

                ImGui::InputFloat4("data1", (float*)& selected.data.data1);
                ImGui::InputFloat4("data2", (float*)& selected.data.data2);
                ImGui::InputFloat4("data3", (float*)& selected.data.data3);
                ImGui::InputFloat4("data4", (float*)& selected.data.data4);
            }
            ImGui::End();

            ImGui::Render();

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

    void immediateSubmit(std::function<void(VkCommandBuffer command)>&& function){
        VK_CHECK(vkResetFences(device, 1, &immediateFence));
        VK_CHECK(vkResetCommandBuffer(immediateCommandBuffer, 0));

        VkCommandBuffer command = immediateCommandBuffer;
        VkCommandBufferBeginInfo commandBeginInfo = Initializers::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(command, &commandBeginInfo));

        function(command);

        VK_CHECK(vkEndCommandBuffer(command));

        VkCommandBufferSubmitInfo submitInfo = Initializers::commandBufferSubmitInfo(command);
        VkSubmitInfo2 submit = Initializers::submitInfo(&submitInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, immediateFence));
        VK_CHECK(vkWaitForFences(device, 1, &immediateFence, true, 9999999999));
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
        
        mainDeletionQueue.flush();

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

        getCurrentFrame().deletionQueue.flush();

        VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

        VkCommandBuffer command = getCurrentFrame().mainCommandBuffer;

        VK_CHECK(vkResetCommandBuffer(command, 0));

        // Set DrawExtent
        drawExtent.width = drawImage.imageExtent.width;
        drawExtent.height = drawImage.imageExtent.height;

        VkCommandBufferBeginInfo beginInfo = Initializers::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(command, &beginInfo));

        // Transition to general layout so we can draw to it
        Utility::transitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        // Draw to drawImage.image
        drawBackground(command);

        Utility::transitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        drawGeometry(command);

        // Transition drawImage to src optimal and swapchain to dst optimal
        Utility::transitionImage(command, drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        Utility::transitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copies drawImage to swapchainImage
        Utility::copyImageToImage(command, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);
        // Transition swapchain to present
        Utility::transitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        drawImgui(command, swapchainImageViews[swapchainImageIndex]);

        Utility::transitionImage(command, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &allocator);

        mainDeletionQueue.pushFunction([&]() {
            vmaDestroyAllocator(allocator);
        });
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

        VkExtent3D drawImageExtent = {
            swapchainExtent.width,
            swapchainExtent.height
        };

        drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        drawImage.imageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsage{};
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageCreateInfo rimageInfo = Initializers::imageCreateInfo(drawImage.imageFormat, drawImageUsage, drawImageExtent);

        VmaAllocationCreateInfo rimageAllocInfo{};
        rimageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        rimageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(allocator, &rimageInfo, &rimageAllocInfo, &drawImage.image, &drawImage.allocation, nullptr));

        VkImageViewCreateInfo rviewInfo = Initializers::imageViewCreateInfo(drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(device, &rviewInfo, nullptr, &drawImage.imageView));

        mainDeletionQueue.pushFunction([=](){
            vkDestroyImageView(device, drawImage.imageView, nullptr);
            vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
            // vkDestroyImage(device, drawImage.image, nullptr);
        });
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
        
        VK_CHECK(vkCreateCommandPool(device, &createInfo, nullptr, &immediateCommandPool));

        VkCommandBufferAllocateInfo commandAllocInfo = Initializers::commandBufferAllocateInfo(immediateCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocInfo, &immediateCommandBuffer));

        mainDeletionQueue.pushFunction([=](){
            vkDestroyCommandPool(device, immediateCommandPool, nullptr);
        });

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

        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immediateFence));
        mainDeletionQueue.pushFunction([=](){
            vkDestroyFence(device, immediateFence, nullptr);
        });
    }

    void drawBackground(VkCommandBuffer command){
        ComputeEffect& effect = backgroundEffects[currentBackgroundEffect];

        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout, 0, 1, &drawImageDescriptors, 0, nullptr);

        vkCmdPushConstants(command, gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
        vkCmdDispatch(command, std::ceil(drawExtent.width/16.0), std::ceil(drawExtent.height/16.0), 1);
    }

    void setupDescriptors(){
        std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
        };

        globalDescriptorAllocator.initPool(device, 10, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            drawImageDescriptorLayout = builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        drawImageDescriptors = globalDescriptorAllocator.allocate(device, drawImageDescriptorLayout);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = drawImage.imageView;

        VkWriteDescriptorSet drawImageInfo{};
        drawImageInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        drawImageInfo.pNext = nullptr;

        drawImageInfo.dstBinding = 0;
        drawImageInfo.dstSet = drawImageDescriptors;
        drawImageInfo.descriptorCount = 1;
        drawImageInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        drawImageInfo.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &drawImageInfo, 0, nullptr);
        
        mainDeletionQueue.pushFunction([&](){
            globalDescriptorAllocator.destroyPool(device);
            vkDestroyDescriptorSetLayout(device, drawImageDescriptorLayout, nullptr);
        });

    }

    // copied, I assume from teh demo as well
    void setupImgui(){
        VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        initInfo.Queue = graphicsQueue;
        initInfo.DescriptorPool = imguiPool;
        initInfo.MinImageCount = 3;
        initInfo.ImageCount = 3;
        initInfo.UseDynamicRendering = true;

        initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;

        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo);
        ImGui_ImplVulkan_CreateFontsTexture();

        mainDeletionQueue.pushFunction([=](){
            ImGui_ImplVulkan_Shutdown();
            vkDestroyDescriptorPool(device, imguiPool, nullptr);
        });
    }

    void drawImgui(VkCommandBuffer command, VkImageView targetImageView){
        VkRenderingAttachmentInfo colorAttachment = Initializers::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingInfo renderInfo = Initializers::renderingInfo(swapchainExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(command, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);

        vkCmdEndRendering(command);
    }

    void setupPipeline(){
        setupBackgroundPipeline();
        // setupTrianglePipeline();
        setupMeshPipeline();
    }

    void setupBackgroundPipeline(){
        VkPipelineLayoutCreateInfo computeLayout{};
        computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext = nullptr;
        computeLayout.pSetLayouts = &drawImageDescriptorLayout;
        computeLayout.setLayoutCount = 1;

        VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(ComputePushConstants);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &gradientPipelineLayout));

        VkShaderModule gradientShader;
        if(!Utility::loadShaderModule("shaders\\gradient.comp.spv", device, &gradientShader)){
            fmt::print("Failed to load gradient Shader!");
        }

        VkShaderModule skyShader;
        if(!Utility::loadShaderModule("shaders\\sky.comp.spv", device, &skyShader)){
            fmt::print("Failed to load sky Shader!");
        }

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;

        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = gradientShader;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.pNext = nullptr;
        computePipelineCreateInfo.layout = gradientPipelineLayout;
        computePipelineCreateInfo.stage = stageInfo;

        ComputeEffect gradient;
        gradient.layout = gradientPipelineLayout;
        gradient.name = "gradient";
        gradient.data = {};
        gradient.data.data1 = glm::vec4(1, 1, 0, 1);
        gradient.data.data2 = glm::vec4(0, 0, 1, 1);

        VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &gradient.pipeline));

        computePipelineCreateInfo.stage.module = skyShader;

        ComputeEffect sky;
        sky.layout = gradientPipelineLayout;
        sky.name = "sky";
        sky.data = {};

        sky.data.data1 = glm::vec4(0.1, 0.2, 0.4 ,0.97);

        VK_CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&computePipelineCreateInfo, nullptr, &sky.pipeline));

        backgroundEffects.push_back(gradient);
        backgroundEffects.push_back(sky);

        vkDestroyShaderModule(device, gradientShader, nullptr);
        vkDestroyShaderModule(device, skyShader, nullptr);
        mainDeletionQueue.pushFunction([&]() {
            vkDestroyPipelineLayout(device, gradientPipelineLayout, nullptr);
            for (size_t i = 0; i < backgroundEffects.size(); i++)
            {
                vkDestroyPipeline(device, backgroundEffects[i].pipeline, nullptr);
            }            
        });
    }

    void drawGeometry(VkCommandBuffer command){
        VkRenderingAttachmentInfo colorAttachment = Initializers::attachmentInfo(drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingInfo renderInfo = Initializers::renderingInfo(drawExtent, &colorAttachment, nullptr);
        vkCmdBeginRendering(command, &renderInfo);

        // vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = drawExtent.width;
        viewport.height = drawExtent.height;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(command, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = drawExtent.width;
        scissor.extent.height = drawExtent.height;

        vkCmdSetScissor(command, 0, 1, &scissor);

        // vkCmdDraw(command, 3, 1, 0, 0);

        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

        GPUDrawPushConstants pushConstants;
        pushConstants.worldMatrix = glm::mat4(1.f);
        pushConstants.vertexBuffer = rectangle.vertexBufferAddress;

        vkCmdPushConstants(command, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);
        vkCmdBindIndexBuffer(command, rectangle.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        
        // HARDCODED TO DRAW 6 VERTICES
        vkCmdDrawIndexed(command, 6, 1, 0, 0, 0);

        vkCmdEndRendering(command);
    }

    void setupTrianglePipeline(){
        VkShaderModule triangleVertShader;
        if(!Utility::loadShaderModule("shaders\\shader.vert.spv", device, &triangleVertShader)){
            fmt::println("Failed to load vertex shader");
        }

        VkShaderModule triangleFragShader;
        if(!Utility::loadShaderModule("shaders\\shader.frag.spv", device, &triangleFragShader)){
            fmt::println("Failed to load frag shader");
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = Initializers::pipelineLayoutCreateInfo();

        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout));

        PipelineBuilder pipelineBuilder;
        pipelineBuilder.pipelineLayout = trianglePipelineLayout;
        pipelineBuilder.setShaders(triangleVertShader, triangleFragShader);
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.disableDepthtest();

        pipelineBuilder.setColorAttachmentFormat(drawImage.imageFormat);
        pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);

        trianglePipeline = pipelineBuilder.buildPipeline(device);

        vkDestroyShaderModule(device, triangleFragShader, nullptr);
        vkDestroyShaderModule(device, triangleVertShader, nullptr);

        mainDeletionQueue.pushFunction([&](){
            vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
            vkDestroyPipeline(device, trianglePipeline, nullptr);
        });
    }

    void setupMeshPipeline(){
        VkShaderModule triangleVertShader;
        if(!Utility::loadShaderModule("shaders\\shader.vert.spv", device, &triangleVertShader)){
            fmt::println("Failed to load vertex shader");
        }

        VkShaderModule triangleFragShader;
        if(!Utility::loadShaderModule("shaders\\shader.frag.spv", device, &triangleFragShader)){
            fmt::println("Failed to load frag shader");
        }

        VkPushConstantRange bufferRange{};
        bufferRange.offset = 0;
        bufferRange.size = sizeof(GPUDrawPushConstants);
        bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo layoutInfo = Initializers::pipelineLayoutCreateInfo();
        layoutInfo.pPushConstantRanges = &bufferRange;
        layoutInfo.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &meshPipelineLayout));

        PipelineBuilder pipelineBuilder;
        pipelineBuilder.pipelineLayout = meshPipelineLayout;
        pipelineBuilder.setShaders(triangleVertShader, triangleFragShader);
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.disableDepthtest();

        pipelineBuilder.setColorAttachmentFormat(drawImage.imageFormat);
        pipelineBuilder.setDepthFormat(VK_FORMAT_UNDEFINED);

        meshPipeline = pipelineBuilder.buildPipeline(device);

        vkDestroyShaderModule(device, triangleFragShader, nullptr);
        vkDestroyShaderModule(device, triangleVertShader, nullptr);

        mainDeletionQueue.pushFunction([&](){
            vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
            vkDestroyPipeline(device, meshPipeline, nullptr);
        });
    }

    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices){
        const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        GPUMeshBuffers newSurface;

        newSurface.vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo deviceAddressInfo{};
        deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
        
        newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

        newSurface.indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void* data = staging.allocation->GetMappedData();

        memcpy(data, vertices.data(), vertexBufferSize);

        memcpy((char*)data+vertexBufferSize, indices.data(), indexBufferSize);

        immediateSubmit([&](VkCommandBuffer command) {
            VkBufferCopy vertexCopy{0};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(command, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy{0};
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(command, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });

        destroyBuffer(staging);

        return newSurface;
    }

    AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage){
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage =  memoryUsage;
        vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer;

        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

        return newBuffer;
    }

    void destroyBuffer(const AllocatedBuffer& buffer){
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }

    void setupDefaultRectangleData(){
        std::array<Vertex,4> rect_vertices;

        rect_vertices[0].position = {0.5,-0.5, 0};
        rect_vertices[1].position = {0.5,0.5, 0};
        rect_vertices[2].position = {-0.5,-0.5, 0};
        rect_vertices[3].position = {-0.5,0.5, 0};

        rect_vertices[0].color = {0,0, 0,1};
        rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
        rect_vertices[2].color = { 1,0, 0,1 };
        rect_vertices[3].color = { 0,1, 0,1 };

        std::array<uint32_t,6> rect_indices;

        rect_indices[0] = 0;
        rect_indices[1] = 1;
        rect_indices[2] = 2;

        rect_indices[3] = 2;
        rect_indices[4] = 1;
        rect_indices[5] = 3;

        rectangle = uploadMesh(rect_indices,rect_vertices);

        //delete the rectangle data on engine shutdown
        mainDeletionQueue.pushFunction([&](){
            destroyBuffer(rectangle.indexBuffer);
            destroyBuffer(rectangle.vertexBuffer);
        });
    }

    void cleanupWindow(){
        glfwDestroyWindow(window);

        glfwTerminate();
    }
};
