#pragma once
#include "utils.h"

class Window{
    public:
        GLFWwindow* window;
        bool frameBufferResized = false;

        Window(){}

        void init() {
            initWindow();
        }

        void cleanupWindow(){
            glfwDestroyWindow(window);

            glfwTerminate();
        }
    
    private:

        void initWindow(){
            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

            window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
            glfwSetWindowUserPointer(window, this); // So any subsequent funtion callbacks use the object
            glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
        }

        static void frameBufferResizeCallback(GLFWwindow* window, int width, int heigh){
            auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            app->frameBufferResized = true;
        }

};