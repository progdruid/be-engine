#include "BeRenderer.h"

#include "BeMaterialArena.h"
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

auto BeRenderer::LaunchDevice() -> void {
    SenBackend::Init({
        #if defined(_DEBUG)
        .DebugLayer = true,
        #endif
    });

    _swapchain = SenBackend::CreateSwapchain({
        .NativeWindowHandle = _nativeWindow,
        .Width = _desiredWidth,
        .Height = _desiredHeight,
    });

    _frameCmd = SenBackend::AllocateCommandBuffer();

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

auto BeRenderer::SetPasses(std::vector<BeRenderPass*> passes) -> void {
    SenBackend::WaitIdle();
    _passes = std::move(passes);
}

auto BeRenderer::ClearPasses() -> void {
    SenBackend::WaitIdle();
    _passes.clear();
}

auto BeRenderer::Render() -> void {
    SenBackend::BeginDebugEvent("Frame");

    _backbufferTexture = SenBackend::BeginFrame(_swapchain);

    // Safe here and not earlier: BeginFrame waits on the fence, so the GPU is done with this slot.
    ++_currentFrame;
    BeMaterialArena::ResetForFrame(_currentFrame);

    _frameCmd.Begin();

    for (const auto& pass : _passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render(*this, _frameCmd);
        SenBackend::EndDebugEvent();
    }

    _frameCmd.TransitionTextures({ { _backbufferTexture, SenResourceState::Present } });
    _frameCmd.End();

    SenBackend::EndFrame(_swapchain, _frameCmd);
    SenBackend::EndDebugEvent();
}

auto BeRenderer::RenderOnce(const std::vector<BeRenderPass*>& passes) -> void {
    SenBackend::BeginDebugEvent("RenderOnce");

    _frameCmd.Begin();

    for (const auto& pass : passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render(*this, _frameCmd);
        SenBackend::EndDebugEvent();
    }

    _frameCmd.End();

    SenBackend::SubmitImmediate(_frameCmd);
    SenBackend::EndDebugEvent();
}
