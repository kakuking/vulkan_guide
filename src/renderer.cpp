#include "utils.h"

#include "vertex.cpp"
#include "queueFamilies.cpp"
#include "swapChainSupportDetails.cpp"
#include "uniformBufferObject.cpp"

#include "window.cpp"
#include "vulkanRenderer.cpp"
#include "debugMessenger.cpp"
#include "surface.cpp"
#include "physicalDevice.cpp"
#include "logicalDevice.cpp"
#include "swapChain.cpp"
#include "renderPass.cpp"
#include "descriptors.cpp"
#include "graphicsPipeline.cpp"
#include "commandBuffer.cpp"
#include "depthBuffer.cpp"
#include "texture.cpp"
#include "geometry.cpp"

class Renderer{
    public:
        void run(){
            window.init();
            initVulkan();
            mainLoop();
            cleanup();
        }


    private:
        Window window;
        VulkanRenderer instance;
        DebugMessenger debugMessenger;
        Surface surface;
        PhysicalDevice physicalDevice;
        LogicalDevice logicalDevice;
        SwapChain swapChain;
        RenderPass renderPass;
        Descriptors descriptors;
        GraphicsPipeline graphicsPipeline;
        CommandBuffer commandBuffer;
        DepthBuffer depthBuffer;
        Texture texture;
        Geometry geometry;
        
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;


        uint32_t currentFrame = 0;

        void initVulkan(){
            instance.setupInstance();
            debugMessenger.setupDebugMessenger(instance.instance);
            surface.setupSurface(instance.instance, window.window);
            physicalDevice.setupPhyiscalDevice(instance.instance, surface.surface);
            logicalDevice.setupLogicalDevice(physicalDevice.physicalDevice, surface.surface);
            swapChain.setupSwapChain(physicalDevice.physicalDevice, surface.surface, logicalDevice.device, window.window);
            swapChain.setupSwapChainImageViews(logicalDevice.device);
            renderPass.setupRenderPass(logicalDevice.device, physicalDevice.physicalDevice, swapChain.swapChainImageFormat);
            descriptors.setupDescriptorSetLayout(logicalDevice.device);
            graphicsPipeline.setupGraphicsPipeline(logicalDevice.device, descriptors.descriptorSetLayout, renderPass.renderPass);
            commandBuffer.setupCommandBuffer(logicalDevice.device, physicalDevice.physicalDevice, surface.surface);
            depthBuffer.setupDepthResources(logicalDevice.device, physicalDevice.physicalDevice, swapChain.swapChainExtent);
            swapChain.setupFrameBuffer(logicalDevice.device, depthBuffer.depthImageView, renderPass.renderPass);
            texture.setupTexture(logicalDevice.graphicsQueue, logicalDevice.device, physicalDevice.physicalDevice, commandBuffer);
            geometry.setupGeometry(logicalDevice.graphicsQueue, logicalDevice.device, physicalDevice.physicalDevice, commandBuffer);
            descriptors.setupUniformBuffersDescriptorPoolsAndSets(logicalDevice.device, physicalDevice.physicalDevice, texture.textureImageView, texture.textureSampler);
            createCommandBuffers();
            createSyncObjects();
        }
        

        bool hasStencilComponent(VkFormat format){
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        void createCommandBuffers(){
            commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

            if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS){
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor); 

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        void createSyncObjects(){
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
                if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create semapgores/fence!");
                }
            }
        }

        void drawFrame(){
            vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

            if(result == VK_ERROR_OUT_OF_DATE_KHR){
                recreateSwapChain();
                return;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            vkResetFences(device, 1, &inFlightFences[currentFrame]);

            // updateVertexData();
            // updateVertexBuffer();

            updateUniformBuffer(currentFrame);

            vkResetCommandBuffer(commandBuffers[currentFrame], 0);
            recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            presentInfo.pResults = nullptr;

            result = vkQueuePresentKHR(presentQueue, &presentInfo);

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.frameBufferResized){
                window.frameBufferResized = false;
                recreateSwapChain();
            } else if (result != VK_SUCCESS){
                throw std::runtime_error("failed to present swap chain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void cleanupSwapChain(){
            vkDestroyImageView(device, depthImageView, nullptr);
            vkDestroyImage(device, depthImage, nullptr);
            vkFreeMemory(device, depthImageMemory, nullptr);

            for(size_t i = 0; i < swapChainFrameBuffers.size(); i++){
                vkDestroyFramebuffer(device, swapChainFrameBuffers[i], nullptr);
            }

            for(size_t i = 0; i < swapChainImageViews.size(); i++){
                vkDestroyImageView(device, swapChainImageViews[i], nullptr);
            }

            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }

        void recreateSwapChain(){
            int width = 0, height = 0;
            glfwGetFramebufferSize(window.window, &width, &height);
            while(width == 0 || height == 0){
                glfwGetFramebufferSize(window.window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);

            cleanupSwapChain();

            createSwapChain();
            createImageViews();
            createDepthResources();
            createFrameBuffer();
        }

        void mainLoop(){
            double totalFrameTime = 0.0;
            auto startTime = std::chrono::high_resolution_clock::now();
            int frameCount = 0;

            while(!glfwWindowShouldClose(window.window)) {
                auto frameStartTime = std::chrono::high_resolution_clock::now();

                glfwPollEvents();
                drawFrame();

                auto frameEndTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> frameDuration = frameEndTime - frameStartTime;
                totalFrameTime += frameDuration.count();
                frameCount++;
            }

            vkDeviceWaitIdle(device);

            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsedSeconds = endTime - startTime;
            double averageFrameTime = totalFrameTime / frameCount;
            double fps = 1000.0 / averageFrameTime;

            std::cout << "Total elapsed time: " << elapsedSeconds.count() << " seconds\n";
            std::cout << "Total frames: " << frameCount << "\n";
            std::cout << "Average frame time: " << averageFrameTime << " ms\n";
            std::cout << "Average FPS: " << fps << "\n";
        }

        void cleanup(){
            cleanupSwapChain();

            vkDestroySampler(device, textureSampler, nullptr);
            vkDestroyImageView(device, textureImageView, nullptr);
            vkDestroyImage(device, textureImage, nullptr);
            vkFreeMemory(device, textureImageMemory, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkDestroyBuffer(device, uniformBuffers[i], nullptr);
                vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            }

            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);

            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);

            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);

            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

            vkDestroyRenderPass(device, renderPass, nullptr);

            for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }

            vkDestroyCommandPool(device, commandPool, nullptr);
            
            vkDestroyDevice(device, nullptr);

            if(enableValidationLayers) {
                debugMessenger.cleanupDebugMessenger(instance.instance);
            }

            surface.cleanupSurface(instance.instance);
            instance.cleanupInstance();
            window.cleanupWindow();
        }
};
