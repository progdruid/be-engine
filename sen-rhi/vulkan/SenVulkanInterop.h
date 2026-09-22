#pragma once
#include <vulkan/vulkan_core.h>

#include "SenTypes.h"


namespace SenVulkanInterop {

auto GetInstance() -> VkInstance;
auto GetPhysicalDevice() -> VkPhysicalDevice;
auto GetDevice() -> VkDevice;
auto GetQueue() -> VkQueue;
auto GetQueueFamilyIndex() -> uint32_t;
auto GetCommandBuffer(SenCommandList list) -> VkCommandBuffer;

}
