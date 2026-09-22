#include "BeRenderer.h"

#include "BeBackend.h"
#include "BePassSequence.h"
#include "BeRenderPass.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
#include <sen-rhi/Sen.h>

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
    BeShaderLibrary::Shutdown();
    BeBackend::Shutdown();
    Sen::Shutdown();
}

auto BeRenderer::LaunchDevice(SenPresentMode presentMode) -> void {
    Sen::Init({
        #if defined(_DEBUG)
        .DebugLayer = true,
        #endif
    });
    BeBackend::Init();
    BeShaderLibrary::Init();

    _swapchain = Sen::CreateSwapchain({
        .NativeWindowHandle = _nativeWindow,
        .Width = _desiredWidth,
        .Height = _desiredHeight,
        .PresentMode = presentMode,
    });

    for (auto& cmd : _frameCmds) {
        cmd = Sen::CreateCommandList(SenQueue::Graphics);
    }
    _immediateCmd = Sen::CreateCommandList(SenQueue::Graphics);

    BeShaderLibrary::RegisterBuiltinDefaultTextures();
    BeShaderLibrary::LoadShaders();
}

auto BeRenderer::GetSwapchainFormat() const -> SenFormat {
    return Sen::GetSwapchainFormat(_swapchain);
}

auto BeRenderer::GetSwapchainPixelWidth() const -> uint32_t {
    return Sen::GetSwapchainWidth(_swapchain);
}

auto BeRenderer::GetSwapchainPixelHeight() const -> uint32_t {
    return Sen::GetSwapchainHeight(_swapchain);
}

auto BeRenderer::GetViewport() const -> SenViewport {
    return { 0, 0, float(GetSwapchainPixelWidth()), float(GetSwapchainPixelHeight()), 0, 1 };
}

auto BeRenderer::SetSequence(BePassSequence* sequence) -> void {
    _sequence = sequence;
}

auto BeRenderer::WaitIdle() -> void {
    Sen::WaitIdle();
}

auto BeRenderer::PollResize() -> bool {
    uint32_t width = 0;
    uint32_t height = 0;
    Sen::GetSurfaceExtent(_swapchain, width, height);

    if (width == 0 || height == 0) { return false; }
    if (width == UINT32_MAX) { return true; }

    if (width != GetSwapchainPixelWidth() || height != GetSwapchainPixelHeight()) {
        Sen::WaitIdle();
        Sen::ResizeSwapchain(_swapchain, width, height);
        _desiredWidth = width;
        _desiredHeight = height;
    }
    return true;
}

auto BeRenderer::Render() -> void {
    be_assert(_sequence != nullptr, "BeRenderer::Render(): sequence is null");
    
    const uint32_t slot = _currentFrame % FramesInFlight;
    if (_frameSubmissions[slot].IsValid()) {
        Sen::WaitForSubmission(_frameSubmissions[slot]);
    }

    // safe here, not earlier: the GPU is done with both the command buffer and the arena chunks
    // about to be reused.
    BeBackend::ResetMaterialArena(_currentFrame);
    BeBackend::FlushRetirements();

    _backbufferView = Sen::AcquireSwapchainView(_swapchain);
    if (!_backbufferView.IsValid() && PollResize()) {
        _backbufferView = Sen::AcquireSwapchainView(_swapchain);
    }
    if (!_backbufferView.IsValid()) {
        return;
    }
    BeBackend::ResetTextureLayouts(Sen::GetViewDesc(_backbufferView).Texture, 1, 1);

    const SenCommandList cmd = _frameCmds[slot];
    SenCmd::Begin(cmd);
    SenCmd::PushMarker(cmd, "Frame");

    for (const auto& pass : _sequence->Passes) {
        SenCmd::PushMarker(cmd, pass->GetPassName().c_str());
        pass->Render(*this, cmd);
        SenCmd::PopMarker(cmd);
    }

    BeBackend::QueueTransition(_backbufferView, SenLayout::Present);
    BeBackend::FlushTransitions(cmd);
    SenCmd::PopMarker(cmd);
    SenCmd::End(cmd);

    const SenSubmission submission = Sen::Submit({
        .Lists = &cmd,
        .ListCount = 1,
        .Presents = &_swapchain,
        .PresentCount = 1,
    });
    _frameSubmissions[slot] = submission;
    BeBackend::StampRetirements(submission);

    ++_currentFrame;
}

auto BeRenderer::RenderOnce(const std::vector<BeRenderPass*>& passes) -> void {
    SenCmd::Begin(_immediateCmd);
    SenCmd::PushMarker(_immediateCmd, "RenderOnce");

    for (const auto& pass : passes) {
        SenCmd::PushMarker(_immediateCmd, pass->GetPassName().c_str());
        pass->Render(*this, _immediateCmd);
        SenCmd::PopMarker(_immediateCmd);
    }

    SenCmd::PopMarker(_immediateCmd);
    SenCmd::End(_immediateCmd);

    const SenSubmission submission = Sen::Submit({ .Lists = &_immediateCmd, .ListCount = 1 });
    Sen::WaitForSubmission(submission);
    BeBackend::StampRetirements(submission);
}
