cmake_minimum_required(VERSION 3.10)
project(VulkanEngine)

set(CMAKE_CXX_STANDARD 20)

# Include directories
include_directories(include)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Add source files
file(GLOB SOURCES 
    src/*.cpp
    src/*.h
    )

# Set the linker flags to link the static C++ standard library
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")

# Create executable
add_executable(VulkanEngine ${SOURCES})

# Find Vulkan package
find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
    message(${Vulkan_INCLUDE_DIRS})
    include_directories(${Vulkan_INCLUDE_DIRS})
    target_link_libraries(VulkanEngine ${Vulkan_LIBRARIES})
else()
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()

# Add subdirectories
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third-party/fmt)
add_subdirectory(third-party/glfw)
add_subdirectory(third-party/glm)
add_subdirectory(third-party/vkbootstrap)

# Ensure GLFW include directories are set
target_include_directories(VulkanEngine PRIVATE third-party/glfw/include)
target_include_directories(VulkanEngine PRIVATE third-party/glm)
target_include_directories(VulkanEngine PRIVATE third-party/fmt/include)

add_library(vma INTERFACE)

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC third-party/imgui)

target_sources(imgui PRIVATE 
    third-party/imgui/imgui.h
    third-party/imgui/imgui.cpp

    third-party/imgui/imgui_demo.cpp
    third-party/imgui/imgui_draw.cpp
    third-party/imgui/imgui_widgets.cpp
    third-party/imgui/imgui_tables.cpp
    third-party/imgui/backends/imgui_impl_vulkan.cpp
    third-party/imgui/backends/imgui_impl_glfw.cpp
    )

target_include_directories(imgui PUBLIC third-party/imgui/backends)
target_link_libraries(imgui PUBLIC ${Vulkan_LIBRARIES} glfw)

# Link libraries
target_include_directories(vma INTERFACE third-party/vma)
target_link_libraries(VulkanEngine glfw fmt glm vk-bootstrap vma imgui)


# Link other necessary libraries
if (WIN32)
    target_link_libraries(VulkanEngine ${CMAKE_DL_LIBS})
    target_link_libraries(VulkanEngine opengl32 gdi32 user32)
elseif(APPLE)
    find_library(COCOA Cocoa)
    find_library(IOKIT IOKit)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(CORE_VIDEO CoreVideo)
    target_link_libraries(VulkanEngine ${COCOA} ${IOKIT} ${CORE_FOUNDATION} ${CORE_VIDEO})
else()
    target_link_libraries(VulkanEngine GL X11 pthread Xrandr Xi dl)
endif()

# Compile shaders to SPIR-V
# file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)      # uncomment to compile shaders to build/shaders

file(GLOB SHADERS "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")

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
add_dependencies(VulkanEngine Shaders)

# Copying Static Folder
set(source_dir "static")
set(dst_dir "${CMAKE_BINARY_DIR}")
file(COPY ${source_dir} DESTINATION ${dst_dir})
