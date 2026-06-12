#include "BeRenderer.h"

#include "BeRenderPass.h"
#include "BeShader.h"
#include <sen-rhi/SenBackend.h>

auto BeRenderer::GetCommandBuffer() -> SenCommandBuffer& {
    return SenBackend::GetCommandBuffer();
}

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

    SenBackend::CreateCommandBuffer();
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

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::ClearPasses() -> void {
    SenBackend::WaitIdle();
    _passes.clear();
}

auto BeRenderer::Render() -> void {
    SenBackend::BeginDebugEvent("Frame");

    _backbufferTexture = SenBackend::BeginFrame(_swapchain);

    for (const auto& pass : _passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render();
        SenBackend::EndDebugEvent();
    }

    SenBackend::EndFrame(_swapchain);
    SenBackend::EndDebugEvent();
}
