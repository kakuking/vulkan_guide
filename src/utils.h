#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

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
#include <deque>
#include <functional>
#include <span>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

const uint32_t WIDTH = 1700, HEIGHT = 900;

const bool USE_VALIDATION_LAYERS = true;

const uint32_t FRAME_OVERLAP = 2; // I think same as MAX_FRAMES_IN_FLIGHT

// MACRO for VK_SUCCESS check
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::println("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)
