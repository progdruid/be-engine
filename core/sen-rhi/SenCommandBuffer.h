#pragma once
#include <sen-rhi/vulkan/SenVulkanCommandBuffer.h>
#include <sen-rhi/SenTypes.h>

using SenCommandBuffer = SenVulkanCommandBuffer;

struct SenSubmitDesc {
    SenCommandBuffer* const* Lists = nullptr;
    uint32_t ListCount = 0;
    const SenSwapchain* Presents = nullptr;
    uint32_t PresentCount = 0;
};
