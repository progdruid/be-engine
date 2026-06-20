#pragma once
#include <array>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

class SenVulkanCommandBuffer {
    hide
    VkCommandBuffer     _cmd                 = VK_NULL_HANDLE;
    VkPipelineLayout    _boundPipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint _boundBindPoint      = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SenPipeline _boundPipeline;
    bool        _pipelineDirty = false;

    static constexpr uint8_t MaxBindGroups = 8;
    std::array<SenBindGroup, MaxBindGroups> _boundGroups           = {};
    std::array<bool,         MaxBindGroups> _boundGroupsDirtyFlags = {};


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
    auto SetPipeline     (SenPipeline pipeline) -> void;
    auto SetBindGroup    (SenBindGroup group, uint8_t index) -> void;
    auto SetVertexBuffer (SenBuffer buffer) -> void;
    auto SetIndexBuffer  (SenBuffer buffer) -> void;

    hide
    auto FlushState() -> void;

    expose
    auto Draw        (uint32_t vertexCount,  uint32_t firstVertex) -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
    auto Dispatch (uint32_t x, uint32_t y, uint32_t z) -> void;
};
