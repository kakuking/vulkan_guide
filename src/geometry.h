#pragma once
#include "utils.h"

#include "vertex.h"
#include "buffer.h"

class Geometry{
    public:
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
        };

        const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4
        };

        void setupGeometry(VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice, CommandBuffer commandBufferManager){
            createVertexBuffer(graphicsQueue, device, physicalDevice, commandBufferManager);
            createIndexBuffer(graphicsQueue, device, physicalDevice, commandBufferManager);
        }

        void cleanupGeometry(VkDevice device){
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);

            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);

            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);
        }

    private:
        void updateVertexData() {
            for (auto& vertex : vertices) {
                vertex.pos.x += 0.01f;
                vertex.pos.y += 0.01f;
            }
        }

        void updateVertexBuffer(VkQueue graphicsQueue, VkDevice device, CommandBuffer commandBufferManager){
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

            // VkBuffer stagingBuffer;
            // VkDeviceMemory stagingBufferMemory;
            
            // createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            Buffer::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, graphicsQueue, device, commandBufferManager);

            // vkDestroyBuffer(device, stagingBuffer, nullptr);
            // vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void createVertexBuffer(VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice, CommandBuffer commandBufferManager){
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

            // VkBuffer stagingBuffer;
            // VkDeviceMemory stagingBufferMemory;

            Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, device, physicalDevice);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, device, physicalDevice);

            Buffer::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, graphicsQueue, device, commandBufferManager);

            // vkDestroyBuffer(device, stagingBuffer, nullptr);
            // vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void createIndexBuffer(VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice, CommandBuffer commandBufferManager){
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, device, physicalDevice);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory, device, physicalDevice);

            Buffer::copyBuffer(stagingBuffer, indexBuffer, bufferSize, graphicsQueue, device, commandBufferManager);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
};