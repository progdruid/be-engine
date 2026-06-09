#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "umbrellas/access-modifiers.hpp"

// Khronos validation layer wiring. No-op in release builds.
class SenVulkanValidation {
    hide
    static VkDebugUtilsMessengerEXT _messenger;
    static VkDebugUtilsMessengerCreateInfoEXT _createInfo;
    static bool _enabled;
    static bool _requested;
    
    expose
    static auto SetEnabled(bool enabled) -> void;
    static auto ConfigureForInstance(std::vector<const char*>& layers, std::vector<const char*>& extensions) -> const void*;
    static auto CreateMessenger(VkInstance instance) -> void;
    static auto DestroyMessenger(VkInstance instance) -> void;

    hide
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData);
    static auto MakeMessengerInfo() -> VkDebugUtilsMessengerCreateInfoEXT;
    static auto LayerAvailable() -> bool;
    static auto ExecutableDir() -> std::string;
};
