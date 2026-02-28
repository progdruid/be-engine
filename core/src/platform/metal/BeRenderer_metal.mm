// System headers before engine headers to avoid access-modifier macro conflicts with AppKit.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include "BeRenderer.h"
#include "BePipeline.h"
#include "BeRendererImpl.h"
#include "MetalUtils.h"

#include "BeBuffers.h"
#include "BeWindow.h"

BeRenderer::BeRenderer(
    uint32_t width,
    uint32_t height,
    BeWindow& window
)
    : _width(width)
    , _height(height)
    , _impl(std::make_unique<BeRendererImpl>())
{
    NSWindow* nsWindow = glfwGetCocoaWindow(window.GetGlfwWindow());
    _impl->metalLayer = [CAMetalLayer layer];
    _impl->metalLayer.frame = nsWindow.contentView.frame;
    [nsWindow.contentView setLayer:_impl->metalLayer];
    [nsWindow.contentView setWantsLayer:YES];
}

BeRenderer::~BeRenderer() = default;

auto BeRenderer::LaunchDevice() -> void {
    _impl->device = MTLCreateSystemDefaultDevice();
    _impl->commandQueue = [_impl->device newCommandQueue];
    _impl->metalLayer.device = _impl->device;
    _impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _impl->metalLayer.drawableSize = CGSizeMake(_width, _height);

    _pipeline = BePipeline::Create(*this);

    _impl->uniformBuffer = [_impl->device newBufferWithLength:sizeof(BeUniformBufferGPU)
                                                      options:MTLResourceStorageModeShared];

    MTLDepthStencilDescriptor* depthDesc = [[MTLDepthStencilDescriptor alloc] init];
    depthDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthDesc.depthWriteEnabled = YES;
    _impl->defaultDepthStencilState = [_impl->device newDepthStencilStateWithDescriptor:depthDesc];

    MTLDepthStencilDescriptor* disabledDesc = [[MTLDepthStencilDescriptor alloc] init];
    disabledDesc.depthCompareFunction = MTLCompareFunctionAlways;
    disabledDesc.depthWriteEnabled = NO;
    _impl->disabledDepthStencilState = [_impl->device newDepthStencilStateWithDescriptor:disabledDesc];
}

auto BeRenderer::Render() -> void {
    @autoreleasepool {
        _impl->currentDrawable = [_impl->metalLayer nextDrawable];
        if (!_impl->currentDrawable) return;

        _impl->currentCommandBuffer = [_impl->commandQueue commandBuffer];

        const BeUniformBufferGPU uniformDataGpu(UniformData);
        memcpy([_impl->uniformBuffer contents], &uniformDataGpu, sizeof(BeUniformBufferGPU));

        for (const auto& pass : _passes) {
            pass->Render();
        }

        if (_impl->currentEncoder) {
            [_impl->currentEncoder endEncoding];
            _impl->currentEncoder = nil;
        }

        [_impl->currentCommandBuffer presentDrawable:_impl->currentDrawable];
        [_impl->currentCommandBuffer commit];

        _pipeline->ClearCache();

        _impl->currentDrawable = nil;
        _impl->currentCommandBuffer = nil;
    }
}
