#pragma once

#include <GLFW/glfw3.h>

#include "utils.h"

class Engine {
public:    
    Engine(){}
    
    void run(){
        setupWindow();

        while(!glfwWindowShouldClose(window)){
            auto frameStartTime = std::chrono::high_resolution_clock::now();

            glfwPollEvents();

            auto frameEndTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> frameDuration = frameEndTime - frameStartTime;

            FPS = 1000.0 / frameDuration.count();
            
            std::stringstream ss;
            ss << "Vulkan | " << FPS << " FPS";
            glfwSetWindowTitle(window, ss.str().c_str());
        }


    }

private:
    GLFWwindow* window;
    double FPS;

    void setupWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void cleanupWindow(){
        glfwDestroyWindow(window);

        glfwTerminate();
    }

};
