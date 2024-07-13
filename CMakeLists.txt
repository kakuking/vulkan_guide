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
    message(${Vulkan_INCLUDE_DIRS})
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

# Compile shaders to SPIR-V
# file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)      # uncomment to compile shaders to build/shaders

file(GLOB SHADERS "shaders/*.vert" "shaders/*.frag")

foreach(SHADER ${SHADERS})
    get_filename_component(FILENAME ${SHADER} NAME)
    # set(SPV "${CMAKE_SOURCE_DIR}/shaders/${FILENAME}.spv")
    set(SPV "${CMAKE_BINARY_DIR}/shaders/${FILENAME}.spv")    # uncomment to compile shaders to build/shaders
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND glslc ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER} to SPIR-V"
        VERBATIM
    )
    list(APPEND SPV_FILES ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})
add_dependencies(VulkanTrial Shaders)

# Copying Static Folder

set(source_dir "static")
set(dst_dir "${CMAKE_BINARY_DIR}")
file(COPY ${source_dir} DESTINATION ${dst_dir})