cmake_minimum_required(VERSION 3.10)
project(VulkanTrial)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(include)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Set the linker flags to link the static C++ standard library
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")

# Create executable
add_executable(VulkanTrial ${SOURCES})

# Find Vulkan package
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    target_link_libraries(VulkanTrial ${Vulkan_LIBRARIES})
else()
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()

# Link GLFW library
target_link_libraries(VulkanTrial glfw3)

# Link other necessary libraries
if (WIN32)
    target_link_libraries(VulkanTrial ${CMAKE_DL_LIBS})
    target_link_libraries(VulkanTrial opengl32 gdi32 user32)
elseif(APPLE)
    find_library(COCOA Cocoa)
    find_library(IOKIT IOKit)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(CORE_VIDEO CoreVideo)
    target_link_libraries(VulkanTrial ${COCOA} ${IOKIT} ${CORE_FOUNDATION} ${CORE_VIDEO})
else()
    target_link_libraries(VulkanTrial GL X11 pthread Xrandr Xi dl)
endif()
