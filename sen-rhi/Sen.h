#pragma once
#include <SenTypes.h>
#include <common.hpp>


class Sen {
    expose
    static constexpr uint32_t MaxSubmitLists = 8;
    static constexpr uint32_t MaxSubmitPresents = 4;
    static constexpr uint32_t MaxSubmitWaits = 8;

    static auto Init(const SenInitDesc& desc) -> void;
    static auto Shutdown() -> void;
    static auto WaitIdle() -> void;
    static auto GetCaps() -> SenCaps;

    static auto CreateBuffer(const SenBufferDesc& desc) -> SenBuffer;
    static auto DestroyBuffer(SenBuffer handle) -> void;
    static auto GetBufferPointer(SenBuffer handle) -> void*;
    static auto GetBufferAddress(SenBuffer handle) -> SenGpuAddress;
    static auto GetBufferMemory(SenBuffer handle) -> SenMemory;

    static auto CreateTexture(const SenTextureDesc& desc) -> SenTexture;
    static auto DestroyTexture(SenTexture handle) -> void;

    static auto CreateView(const SenViewDesc& desc) -> SenView;
    static auto DestroyView(SenView handle) -> void;
    static auto GetViewDesc(SenView handle) -> const SenViewDesc&;
    static auto GetViewFormat(SenView handle) -> SenFormat;

    static auto CreateSampler(const SenSamplerDesc& desc) -> SenSampler;
    static auto DestroySampler(SenSampler handle) -> void;

    static auto PublishTextureBindless(uint32_t slot, SenView view) -> void;
    static auto PublishStorageBindless(uint32_t slot, SenView view) -> void;
    static auto PublishSamplerBindless(uint32_t slot, SenSampler sampler) -> void;

    static auto CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline(SenPipeline handle) -> void;

    static auto CreateCommandList(SenQueue queue) -> SenCommandList;
    static auto DestroyCommandList(SenCommandList handle) -> void;

    static auto Submit(const SenSubmitDesc& desc) -> SenSubmission;
    static auto IsSubmissionComplete(SenSubmission submission) -> bool;
    static auto WaitForSubmission(SenSubmission submission) -> void;

    static auto CreateSwapchain(const SenSwapchainDesc& desc) -> SenSwapchain;
    static auto DestroySwapchain(SenSwapchain handle) -> void;
    static auto ResizeSwapchain(SenSwapchain& handle, uint32_t width, uint32_t height) -> void;
    static auto AcquireSwapchainView(SenSwapchain handle) -> SenView;
    static auto GetSwapchainFormat(SenSwapchain handle) -> SenFormat;
    static auto GetSwapchainWidth(SenSwapchain handle) -> uint32_t;
    static auto GetSwapchainHeight(SenSwapchain handle) -> uint32_t;
    static auto GetSurfaceExtent(SenSwapchain handle, uint32_t& outWidth, uint32_t& outHeight) -> void;
};

class SenCmd {
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
};
