#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

struct BeRendererImpl {
#ifdef __OBJC__
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    CAMetalLayer* metalLayer = nil;
    id<CAMetalDrawable> currentDrawable = nil;
    id<MTLCommandBuffer> currentCommandBuffer = nil;
    id<MTLRenderCommandEncoder> currentEncoder = nil;

    id<MTLBuffer> uniformBuffer = nil;
    id<MTLDepthStencilState> defaultDepthStencilState = nil;
    id<MTLDepthStencilState> disabledDepthStencilState = nil;

    MTLCullMode currentCullMode = MTLCullModeBack;
#else
    void* device = nullptr;
    void* commandQueue = nullptr;
    void* metalLayer = nullptr;
    void* currentDrawable = nullptr;
    void* currentCommandBuffer = nullptr;
    void* currentEncoder = nullptr;
    void* uniformBuffer = nullptr;
    void* defaultDepthStencilState = nullptr;
    void* disabledDepthStencilState = nullptr;
#endif
};
