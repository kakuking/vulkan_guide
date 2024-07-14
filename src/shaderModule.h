#pragma once

#include "utils.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class ShaderModule{
    public:
        static VkShaderModule createShaderModule(const std::string& filename, VkDevice device){
            auto shaderCode = readFile(filename);

            return createShaderModule(shaderCode, device);
        }

    private:
        static std::vector<char> readFile(const std::string& filename){
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if(!file.is_open()){
                std::stringstream errorMsg;
                errorMsg << "Failed to open file: " << filename;
                throw std::runtime_error(errorMsg.str());
            }

            size_t fileSize = (size_t) file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }

        static VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
            
            VkShaderModule shaderModule;
            if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module");
            }

            return shaderModule;
        }

};