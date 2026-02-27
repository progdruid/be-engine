#ifdef __APPLE__

#import "MetalRenderer.h"
#import "MetalContext.h"
#import "MetalSwapchain.h"
#import "MetalTexture.h"
#import <Metal/Metal.h>

#include "BeRenderPass.h"

MetalRenderer::MetalRenderer(uint32_t width, uint32_t height, void* nativeView)
    : _width(width)
    , _height(height)
    , _nativeView(nativeView)
    , _device(nil)
    , _uniformBuffer(nil)
{}

MetalRenderer::~MetalRenderer() {
    ClearPasses();
    _uniformBuffer = nil;
    _device = nil;
}

auto MetalRenderer::LaunchDevice() -> void {
    _device = MTLCreateSystemDefaultDevice();

    _context = std::make_shared<MetalContext>(_device);
    _swapchain = std::make_shared<MetalSwapchain>(_device, _nativeView, _width, _height);

    _uniformBuffer = [_device newBufferWithLength:sizeof(BeUniformBufferGPU)
                                          options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];

    MetalTexture::Descriptor depthDesc;
    depthDesc.Name = "depth";
    depthDesc.Width = _width;
    depthDesc.Height = _height;
    depthDesc.Format = RhiFormat::D32_FLOAT;
    depthDesc.BindFlags = RhiBindFlags::DepthStencil;
    _depthTexture = MetalTexture::Create(_device, depthDesc);
}

auto MetalRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(nullptr); // TODO: needs adaptation for Metal renderer
}

auto MetalRenderer::ClearPasses() -> void {
    for (auto pass : _passes) {
        delete pass;
    }
    _passes.clear();
}

auto MetalRenderer::InitialisePasses() const -> void {
    for (const auto& pass : _passes)
        pass->Initialise();
}

auto MetalRenderer::Render() -> void {
    id<MTLCommandBuffer> commandBuffer = _context->BeginFrame();

    BeUniformBufferGPU uniformDataGpu(UniformData);
    memcpy([_uniformBuffer contents], &uniformDataGpu, sizeof(BeUniformBufferGPU));

    id<CAMetalDrawable> drawable = _swapchain->GetCurrentDrawable();
    if (!drawable) return;

    // Each render pass will set up its own MTLRenderPassDescriptor
    // and call context->BeginRenderPass / EndRenderPass
    // For the basic case, we create a simple pass that renders to the backbuffer
    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

    if (_depthTexture) {
        passDesc.depthAttachment.texture = _depthTexture->GetNative();
        passDesc.depthAttachment.loadAction = MTLLoadActionClear;
        passDesc.depthAttachment.storeAction = MTLStoreActionDontCare;
        passDesc.depthAttachment.clearDepth = 1.0;
    }

    _context->BeginRenderPass(passDesc);

    RhiViewport viewport;
    viewport.Width = static_cast<float>(_width);
    viewport.Height = static_cast<float>(_height);
    _context->SetViewport(viewport);

    // TODO: Execute render passes through the abstraction
    // Currently the passes are designed around BeRenderer (DX11)
    // They need to be adapted to work with either backend

    _context->EndRenderPass();

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

#endif
