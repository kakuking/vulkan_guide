#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream> // for printing to cout
#include <fmt/format.h> // Print using fmt::print

#include <chrono> // For FPS
#include <string>
#include <sstream> // for string stream (formatting)

#include <VkBootstrap.h>  // Vulkan Bootstrapper
#include <optional> // Used in vkb

#include <vector>   // Using vectors in places including swapchain images and views

const uint32_t WIDTH = 800, HEIGHT = 600;

const bool USE_VALIDATION_LAYERS = true;