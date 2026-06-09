#include "SenVulkanValidation.h"

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#if defined(_WIN32)
  #include <windows.h>
#elif defined(__linux__)
  #include <unistd.h>
#endif

#include <umbrellas/include-libassert.h>

namespace {
VkDebugUtilsMessengerEXT           g_messenger  = VK_NULL_HANDLE;
VkDebugUtilsMessengerCreateInfoEXT g_createInfo = {};
bool                               g_enabled    = false;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(stderr, "[vulkan] %s\n", data->pMessage);
    }
    // Errors abort right here so libassert gives a stack trace at the offending Vulkan call.
    be_assert(severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, data->pMessage);
    return VK_FALSE;  // VK_FALSE: don't abort the triggering Vulkan call itself
}

VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerInfo() {
    return VkDebugUtilsMessengerCreateInfoEXT {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VulkanDebugCallback,
    };
}

bool ValidationLayerAvailable() {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& l : layers) {
        if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) { return true; }
    }
    return false;
}

// Directory containing the running executable (validation layer is copied next to it under /layers).
std::string ExecutableDir() {
    #if defined(_WIN32)
    {
        char buf[MAX_PATH];
        DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        std::string path(buf, n);
        const auto slash = path.find_last_of("\\/");
        return slash == std::string::npos ? "." : path.substr(0, slash);
    }
    #elif defined(__linux__)
    {    
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf));
        if (n <= 0) { return "."; }
        std::string path(buf, size_t(n));
        const auto slash = path.find_last_of('/');
        return slash == std::string::npos ? "." : path.substr(0, slash);
    }
    #else
    {
        return ".";
    }
    #endif
}
}

auto Sen::Vulkan::Validation::ConfigureInstance(std::vector<const char*>& layers, std::vector<const char*>& extensions) -> const void* {
    #ifdef DEBUG
    {
        std::string layerPath = ExecutableDir() + "/layers";
        const char* existing = std::getenv("VK_ADD_LAYER_PATH");
        #ifdef _WIN32
        {
            if (existing && *existing) {
                layerPath += ';'; layerPath += existing;
            }
            _putenv_s("VK_ADD_LAYER_PATH", layerPath.c_str());
        }  
        #else
        {
            if (existing && *existing) {
                layerPath += ':'; layerPath += existing;
            }
            setenv("VK_ADD_LAYER_PATH", layerPath.c_str(), 1);
        }
        #endif
        
        if (ValidationLayerAvailable()) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            g_createInfo = MakeDebugMessengerInfo();
            g_enabled    = true;
            return &g_createInfo;
        }
        std::fprintf(stderr, "[vulkan] VK_LAYER_KHRONOS_validation not found; validation disabled.\n");
    }
    #endif
    return nullptr;
}

auto Sen::Vulkan::Validation::CreateMessenger(VkInstance instance) -> void {
    if (!g_enabled) {
        return;
    }
    auto create = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    be_assert(create, "Failed to load vkCreateDebugUtilsMessengerEXT");
    VkResult result = create(instance, &g_createInfo, nullptr, &g_messenger);
    be_assert(result == VK_SUCCESS, "Failed to create debug messenger!");
}

auto Sen::Vulkan::Validation::DestroyMessenger(VkInstance instance) -> void {
    if (!g_messenger) {
        return;
    }
    const auto destroy = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroy) { destroy(instance, g_messenger, nullptr); }
    g_messenger = VK_NULL_HANDLE;
}
