#pragma once
#include <vulkan/vulkan_core.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

class SenVulkanCommandBuffer {
    hide
    VkCommandBuffer  _cmd                 = VK_NULL_HANDLE;
    VkPipelineLayout _boundPipelineLayout = VK_NULL_HANDLE;

    expose
    SenVulkanCommandBuffer() = default;
    explicit SenVulkanCommandBuffer(VkCommandBuffer cmd);

    // render pass
    auto BeginPass (const SenPassDesc& desc) -> void;
    auto EndPass   () -> void;

    // pipeline + resources
    auto SetPipeline     (SenPipeline pipeline) -> void;
    auto SetBindGroup    (SenBindGroup group, uint8_t index) -> void;
    auto SetVertexBuffer (SenBuffer buffer, uint32_t stride) -> void;
    auto SetIndexBuffer  (SenBuffer buffer) -> void;
    auto ClearVertexBuffer () -> void;
    auto ClearIndexBuffer  () -> void;

    // draw
    auto Draw        (uint32_t vertexCount,  uint32_t firstVertex) -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
};
