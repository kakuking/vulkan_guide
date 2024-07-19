#pragma once

#include "utils.h"
#include <fstream>
#include "initializers.h"

namespace Utility{
    std::vector<char> readFile(const std::string& filename){
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

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device) {
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
    
    bool loadShaderModule(const char* filename, VkDevice device, VkShaderModule* outShaderModule){
        std::vector<char> buffer = readFile(filename);

        *outShaderModule = createShaderModule(buffer, device);
        return true;
    }
};