#include "BeRenderer.h"

#include "BeRenderPass.h"
#include "BeShader.h"
#include <sen-rhi/SenBackend.h>

auto BeRenderer::GetCommandBuffer() -> SenCommandBuffer& {
    return SenBackend::GetCommandBuffer();
}

BeRenderer::BeRenderer(
    uint32_t width,
    uint32_t height,
    void* nativeWindow
)
    : _width(width)
    , _height(height)
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
        .Width  = _width,
        .Height = _height,
    });

    SenBackend::CreateCommandBuffer();
}

auto BeRenderer::GetSwapchainFormat() const -> SenFormat {
    return SenBackend::GetSwapchainFormat(_swapchain);
}

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::ClearPasses() -> void {
    SenBackend::WaitIdle();
    for (auto pass : _passes) {
        delete pass;
    }
    _passes.clear();
}

auto BeRenderer::InitialisePasses() const -> void {
    for (const auto& pass : _passes)
        pass->Initialise();
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
