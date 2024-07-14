#pragma once
#include "utils.h"

class Surface{
    public:
        VkSurfaceKHR surface;

        Surface(){}

        void setupSurface(VkInstance instance, GLFWwindow* window) {
            createSurface(instance, window);
        }

        void cleanupSurface(VkInstance instance){
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
    
    private:
        void createSurface(VkInstance instance, GLFWwindow* window){
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Window surface");
            }
        }
};