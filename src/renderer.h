#pragma once
#include "utils.h"

#include "vertex.h"
#include "queueFamilies.h"
#include "swapChainSupportDetails.h"
#include "uniformBufferObject.h"

#include "window.h"
#include "vulkanRenderer.h"
#include "debugMessenger.h"
#include "surface.h"
#include "physicalDevice.h"
#include "logicalDevice.h"
#include "swapChain.h"
#include "renderPass.h"
#include "descriptors.h"
#include "graphicsPipeline.h"
#include "commandBuffer.h"
#include "depthBuffer.h"
#include "texture.h"
#include "geometry.h"
#include "sync.h"

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
        Sync sync;
        

        void initVulkan(){
            // Nothing to Change
            instance.setupInstance();
            debugMessenger.setupDebugMessenger(instance.instance);
            surface.setupSurface(instance.instance, window.window);
            physicalDevice.setupPhyiscalDevice(instance.instance, surface.surface);
            logicalDevice.setupLogicalDevice(physicalDevice.physicalDevice, surface.surface);
            swapChain.setupSwapChain(physicalDevice.physicalDevice, surface.surface, logicalDevice.device, window.window);
            swapChain.setupSwapChainImageViews(logicalDevice.device);
            renderPass.setupRenderPass(logicalDevice.device, physicalDevice.physicalDevice, swapChain.swapChainImageFormat);

            // Things I can append to
            texture.filenames.push_back("static\\texture_0.jpg");
            texture.filenames.push_back("static\\texture_1.jpg");
            texture.filenames.push_back("static\\texture_2.jpg"); 

            descriptors.setupDescriptorSetLayout(logicalDevice.device, texture);

            // Nothing to Change
            commandBuffer.setupCommandBuffer(logicalDevice.device, physicalDevice.physicalDevice, surface.surface);
            depthBuffer.setupDepthResources(logicalDevice.device, physicalDevice.physicalDevice, swapChain.swapChainExtent);
            swapChain.setupFrameBuffer(logicalDevice.device, depthBuffer.depthImageView, renderPass.renderPass);

            // Things to append to
            texture.setupTexture(logicalDevice.graphicsQueue, logicalDevice.device, physicalDevice.physicalDevice, commandBuffer);
            descriptors.setupUniformBuffersDescriptorPoolsAndSets(logicalDevice.device, physicalDevice.physicalDevice, texture.textureImageViews, texture.textureSamplers);
            geometry.setupGeometry(logicalDevice.graphicsQueue, logicalDevice.device, physicalDevice.physicalDevice, commandBuffer);

            // Nothing to change
            graphicsPipeline.setupGraphicsPipeline(logicalDevice.device, descriptors.descriptorSetLayout, renderPass.renderPass);
            commandBuffer.createCommandBuffers(logicalDevice.device);
            sync.setupSyncObjects(logicalDevice.device);
        }

        void drawFrame(
            LogicalDevice logicalDevice,
            VkPhysicalDevice physicalDevice,
            VkSurfaceKHR surface,
            Window& window,
            DepthBuffer& depthBuffer,
            CommandBuffer& commandBuffer,
            VkRenderPass renderPass,
            Descriptors& descriptors,
            VkPipeline& GraphicsPipeline,
            std::vector<VkSemaphore>& imageAvailableSemaphores, 
            std::vector<VkSemaphore>& renderFinishedSemaphores,
            std::vector<VkFence>& inFlightFences,
            uint32_t& currentFrame,
            SwapChain& swapChain,
            Geometry& geometry
            ){
            vkWaitForFences(logicalDevice.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(logicalDevice.device, swapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

            if(result == VK_ERROR_OUT_OF_DATE_KHR){
                swapChain.recreateSwapChain(logicalDevice.device, physicalDevice, surface, window.window, depthBuffer, renderPass);
                return;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            vkResetFences(logicalDevice.device, 1, &inFlightFences[currentFrame]);

            // updateVertexData();
            // updateVertexBuffer();

            descriptors.updateUniformBuffer(currentFrame, swapChain.swapChainExtent);

            vkResetCommandBuffer(commandBuffer.commandBuffers[currentFrame], 0);
            commandBuffer.recordCommandBuffer(commandBuffer.commandBuffers[currentFrame], imageIndex, renderPass, swapChain.swapChainFrameBuffers, swapChain.swapChainExtent, graphicsPipeline.graphicsPipeline, geometry.vertexBuffer, geometry.indexBuffer, geometry.indices, graphicsPipeline.pipelineLayout, descriptors.descriptorSets);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer.commandBuffers[currentFrame];

            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(logicalDevice.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain.swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            presentInfo.pResults = nullptr;

            result = vkQueuePresentKHR(logicalDevice.presentQueue, &presentInfo);

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.frameBufferResized){
                window.frameBufferResized = false;
                swapChain.recreateSwapChain(logicalDevice.device, physicalDevice, surface, window.window, depthBuffer, renderPass);
            } else if (result != VK_SUCCESS){
                throw std::runtime_error("failed to present swap chain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void mainLoop(){
            double totalFrameTime = 0.0;
            auto startTime = std::chrono::high_resolution_clock::now();
            int frameCount = 0;

            while(!glfwWindowShouldClose(window.window)) {
                auto frameStartTime = std::chrono::high_resolution_clock::now();

                glfwPollEvents();
                drawFrame(logicalDevice, physicalDevice.physicalDevice, surface.surface, window, depthBuffer, commandBuffer, renderPass.renderPass, descriptors, graphicsPipeline.graphicsPipeline, sync.imageAvailableSemaphores, sync.renderFinishedSemaphores, sync.inFlightFences, commandBuffer.currentFrame, swapChain, geometry);

                auto frameEndTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> frameDuration = frameEndTime - frameStartTime;
                totalFrameTime += frameDuration.count();
                frameCount++;
            }

            vkDeviceWaitIdle(logicalDevice.device);

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
            swapChain.cleanupSwapChain(logicalDevice.device, depthBuffer);
            texture.cleanupTexture(logicalDevice.device);
            descriptors.cleanupDescriptors(logicalDevice.device);
            geometry.cleanupGeometry(logicalDevice.device);
            graphicsPipeline.cleanupGraphicsPipeline(logicalDevice.device);
            renderPass.cleanupRenderPass(logicalDevice.device);
            sync.cleanupSyncObjects(logicalDevice.device);
            commandBuffer.cleanupCommandBuffer(logicalDevice.device);
            logicalDevice.cleanupLogicalDevice();

            if(enableValidationLayers) {
                debugMessenger.cleanupDebugMessenger(instance.instance);
            }

            surface.cleanupSurface(instance.instance);
            instance.cleanupInstance();
            window.cleanupWindow();
        }
};
