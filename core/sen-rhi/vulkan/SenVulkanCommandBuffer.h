#pragma once
#include <array>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

class SenVulkanCommandBuffer {
    hide
    VkCommandBuffer     _cmd                 = VK_NULL_HANDLE;
    VkPipelineLayout    _boundPipelineLayout = VK_NULL_HANDLE;
    SenPipeline _boundPipeline;
    bool        _pipelineDirty    = false;

    expose
    SenVulkanCommandBuffer() = default;
    explicit SenVulkanCommandBuffer(VkCommandBuffer cmd);

    auto GetNativeHandle() const -> VkCommandBuffer { return _cmd; }

    expose
    auto Begin() -> void;
    auto End()   -> void;

    hide
    auto ResetPerFrameState() -> void;

    expose
    struct TextureTransition {
        static constexpr uint32_t AllMips = UINT32_MAX;
        SenTexture Texture;
        SenResourceState State;
        uint32_t BaseMip = 0;
        uint32_t MipCount = AllMips;
    };
    auto TransitionTextures (const std::vector<TextureTransition>& transitions) -> void;
    auto BeginPass (const SenPassDesc& desc) -> void;
    auto EndPass   () -> void;

    expose
    auto CopyBuffer (SenBuffer src, uint32_t srcOffset, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void;

    expose
    auto SetPipeline     (SenPipeline pipeline) -> void;
    auto PushRoot        (const void* data, uint32_t size) -> void;
    auto SetVertexBuffer (SenBuffer buffer) -> void;
    auto SetIndexBuffer  (SenBuffer buffer) -> void;

    hide
    auto FlushState() -> void;

    expose
    auto Draw        (uint32_t vertexCount,  uint32_t firstVertex) -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
    auto Dispatch (uint32_t x, uint32_t y, uint32_t z) -> void;
};
