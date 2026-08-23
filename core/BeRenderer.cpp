#include "BeRenderer.h"

#include "BeMaterialArena.h"
#include "BePassSequence.h"
#include "BeRenderPass.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
#include <sen-rhi/SenBackend.h>

uint64_t BeRenderer::_currentFrame = 0;

BeRenderer::BeRenderer(
    uint32_t desiredWidth,
    uint32_t desiredHeight,
    void* nativeWindow
)
    : _desiredWidth(desiredWidth)
    , _desiredHeight(desiredHeight)
    , _nativeWindow(nativeWindow)
{}

BeRenderer::~BeRenderer() {
    BeMaterialArena::DestroyAll();
    SenBackend::Shutdown();
}

auto BeRenderer::LaunchDevice(SenPresentMode presentMode) -> void {
    SenBackend::Init({
        #if defined(_DEBUG)
        .DebugLayer = true,
        #endif
    });

    _swapchain = SenBackend::CreateSwapchain({
        .NativeWindowHandle = _nativeWindow,
        .Width = _desiredWidth,
        .Height = _desiredHeight,
        .FramesInFlight = FramesInFlight,
        .PresentMode = presentMode,
    });

    for (auto& cmd : _frameCmds) {
        cmd = SenBackend::AllocateCommandBuffer();
    }
    _immediateCmd = SenBackend::AllocateCommandBuffer();

    BeShaderLibrary::RegisterBuiltinDefaultTextures();
    BeShaderLibrary::LoadShaders();
}

auto BeRenderer::GetSwapchainFormat() const -> SenFormat {
    return SenBackend::GetSwapchainFormat(_swapchain);
}

auto BeRenderer::GetSwapchainPixelWidth() const -> uint32_t {
    return SenBackend::GetSwapchainWidth(_swapchain);
}

auto BeRenderer::GetSwapchainPixelHeight() const -> uint32_t {
    return SenBackend::GetSwapchainHeight(_swapchain);
}

auto BeRenderer::GetViewport() const -> SenViewport {
    return { 0, 0, float(GetSwapchainPixelWidth()), float(GetSwapchainPixelHeight()), 0, 1 };
}

auto BeRenderer::SetSequence(BePassSequence* sequence) -> void {
    _sequence = sequence;
}

auto BeRenderer::WaitIdle() -> void {
    SenBackend::WaitIdle();
}

auto BeRenderer::PollResize() -> bool {
    uint32_t width = 0;
    uint32_t height = 0;
    SenBackend::GetSurfaceExtent(_swapchain, width, height);

    if (width == 0 || height == 0) { return false; }
    if (width == UINT32_MAX) { return true; }

    if (width != GetSwapchainPixelWidth() || height != GetSwapchainPixelHeight()) {
        SenBackend::WaitIdle();
        SenBackend::ResizeSwapchain(_swapchain, width, height);
        _desiredWidth = width;
        _desiredHeight = height;
    }
    return true;
}

auto BeRenderer::Render() -> void {
    be_assert(_sequence != nullptr, "BeRenderer::Render(): sequence is null");
    
    SenBackend::BeginDebugEvent("Frame");

    const uint32_t slot = _currentFrame % FramesInFlight;
    _backbufferTexture = SenBackend::BeginFrame(_swapchain, slot);
    if (!_backbufferTexture.IsValid() && PollResize()) {
        _backbufferTexture = SenBackend::BeginFrame(_swapchain, slot);
    }
    if (!_backbufferTexture.IsValid()) {
        SenBackend::EndDebugEvent();
        return;
    }

    // safe here, not earlier: BeginFrame waits on this slot's fence, so the GPU is done with
    // both the command buffer and the arena chunks about to be reused.
    BeMaterialArena::ResetForFrame(_currentFrame);

    auto& cmd = _frameCmds[slot];
    cmd.Begin();

    for (const auto& pass : _sequence->Passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render(*this, cmd);
        SenBackend::EndDebugEvent();
    }

    cmd.TransitionTextures({ { _backbufferTexture, SenResourceState::Present } });
    cmd.End();

    SenBackend::EndFrame(_swapchain, cmd, slot);
    SenBackend::EndDebugEvent();

    ++_currentFrame;
}

auto BeRenderer::RenderOnce(const std::vector<BeRenderPass*>& passes) -> void {
    SenBackend::BeginDebugEvent("RenderOnce");

    _immediateCmd.Begin();

    for (const auto& pass : passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render(*this, _immediateCmd);
        SenBackend::EndDebugEvent();
    }

    _immediateCmd.End();

    SenBackend::SubmitImmediate(_immediateCmd);
    SenBackend::EndDebugEvent();
}
