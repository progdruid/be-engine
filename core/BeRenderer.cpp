#include "BeRenderer.h"

#include <cassert>

#include "BeRenderPass.h"
#include "BeShader.h"
#include <sen-rhi/SenBackend.h>

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

    _commandBuffer = SenBackend::CreateCommandBuffer();

    _uniformBuffer = SenBackend::CreateBuffer({
        .Usage  = SenBufferUsage::Constant,
        .Access = SenBufferAccess::Dynamic,
        .Size   = sizeof(BeUniformBufferGPU),
    });

    _uniformBindGroupLayout = SenBackend::CreateBindGroupLayout({
        .BufferSlots = { 0 }
    });

    _uniformBindGroup = SenBackend::CreateBindGroup({
        .Layout = _uniformBindGroupLayout,
        .ConstantBuffers = {_uniformBuffer },
    });
}

auto BeRenderer::GetSwapchainFormat() const -> SenFormat {
    return SenBackend::GetSwapchainFormat(_swapchain);
}

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::ClearPasses() -> void {
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

    const BeUniformBufferGPU uniformDataGpu(UniformData);
    SenBackend::WriteBuffer(_uniformBuffer, &uniformDataGpu, sizeof(BeUniformBufferGPU));

    _commandBuffer.SetBindGroup(_uniformBindGroup, 0);

    for (const auto& pass : _passes) {
        SenBackend::BeginDebugEvent(std::string(pass->GetPassName()));
        pass->Render();
        SenBackend::EndDebugEvent();
    }

    SenBackend::EndFrame(_swapchain);
    SenBackend::EndDebugEvent();
}

