#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

struct SenVulkanCommandListEntry {
    VkCommandBuffer  Cmd                 = VK_NULL_HANDLE;
    VkPipelineLayout BoundPipelineLayout = VK_NULL_HANDLE;
    SenPipeline      BoundPipeline;
    bool             PipelineDirty       = false;

    SenCull Cull = SenCull::Back;
    SenFrontFace FrontFace = SenFrontFace::Clockwise;
    SenDepthState Depth;
    float DepthBiasConstant = 0.f;
    float DepthBiasSlope = 0.f;

    bool CullDirty = true;
    bool FrontFaceDirty = true;
    bool DepthDirty = true;
    bool DepthBiasDirty = true;
};

struct SenVulkanCmd {
    expose
    static auto Begin(SenCommandList list) -> void;
    static auto End(SenCommandList list) -> void;

    static auto PushMarker(SenCommandList list, const char* label) -> void;
    static auto PopMarker(SenCommandList list) -> void;

    static auto BeginPass(SenCommandList list, const SenRenderPassDesc& desc) -> void;
    static auto EndPass(SenCommandList list) -> void;

    static auto SetPipeline(SenCommandList list, SenPipeline pipeline) -> void;
    static auto SetRoot(SenCommandList list, const void* data, uint32_t size) -> void;
    static auto SetCull(SenCommandList list, SenCull mode) -> void;
    static auto SetFrontFace(SenCommandList list, SenFrontFace frontFace) -> void;
    static auto SetDepth(SenCommandList list, const SenDepthState& depth) -> void;
    static auto SetDepthBias(SenCommandList list, float constant, float slopeScaled) -> void;
    static auto SetVertexBuffer(SenCommandList list, SenBuffer buffer) -> void;
    static auto SetIndexBuffer(SenCommandList list, SenBuffer buffer) -> void;

    static auto Draw(SenCommandList list, uint32_t vertexCount, uint32_t firstVertex) -> void;
    static auto DrawIndexed(SenCommandList list, uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
    static auto Dispatch(SenCommandList list, uint32_t x, uint32_t y, uint32_t z) -> void;

    static auto Transition(SenCommandList list, const SenTransition* transitions, uint32_t count) -> void;
    static auto CopyBuffer(SenCommandList list, SenBuffer src, uint32_t srcOffset, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void;
    static auto CopyBufferToTexture(SenCommandList list, SenBuffer src, uint32_t srcOffset, SenTexture dst, uint32_t mip) -> void;

    expose // Vulkan interop, to be sealed behind SenVulkanInterop.h
    static auto GetNativeHandle(SenCommandList list) -> VkCommandBuffer;

    hide
    static auto FlushPipeline(SenVulkanCommandListEntry& entry) -> void;
    static auto FlushDrawState(SenVulkanCommandListEntry& entry) -> void;
};
