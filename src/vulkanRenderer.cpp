#include "utils.h"

#include "debugMessenger.cpp"

class VulkanRenderer{
    public:
        VkInstance instance;

        VulkanRenderer(){}

        void setupInstance(){
            createInstance();
        }

        void cleanupInstance(){
            vkDestroyInstance(instance, nullptr);
        }

    private:
        void createInstance(){
            if (enableValidationLayers && !checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            VkApplicationInfo appInfo{};

            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Hey Triangle";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

            if(enableValidationLayers){
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();

                DebugMessenger::populateDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
            } else {
                createInfo.enabledLayerCount = 0;
            }

            auto extensions = getRequiredExtensions();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

            if(result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Instance");
            }
        }

        std::vector<const char*> getRequiredExtensions() {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if(enableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            return extensions;
        }

        bool checkValidationLayerSupport() {
            uint32_t layerCount;
            VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            if (result != VK_SUCCESS) {
                std::cerr << "Failed to enumerate instance layer properties: " << result << std::endl;
                return false;
            }

            std::vector<VkLayerProperties> availableLayers(layerCount);

            result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
            if (result != VK_SUCCESS) {
                std::cerr << "Failed to enumerate instance layer properties with data: " << result << std::endl;
                return false;
            }

            for (const char* layerName: validationLayers) {
                bool layerFound = false;

                for(const auto& layerProp: availableLayers) {
                    if(strcmp(layerName, layerProp.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    std::cerr << "Validation layer not found: " << layerName << std::endl;
                    return false;
                }
            }

            return true;
        }
};