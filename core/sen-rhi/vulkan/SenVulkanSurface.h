#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "sen-rhi/SenTypes.h"

namespace SenVulkanSurface {
    auto ConfigureForInstance(std::vector<const char*>& extensions) -> void;
    auto Create(VkInstance instance, const SenNativeWindow& window) -> VkSurfaceKHR;
}
