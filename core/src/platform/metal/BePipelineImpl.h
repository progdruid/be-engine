#pragma once

#include <unordered_map>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

struct BeRendererImpl;

struct BePipelineImpl {
    BeRendererImpl* rendererImpl = nullptr;

#ifdef __OBJC__
    id<MTLDevice> device = nil;
    id<MTLRenderPipelineState> currentPipelineState = nil;
    id<MTLDepthStencilState> currentDepthStencilState = nil;
    id<MTLBlendColorAttachmentDescriptor> blendDescriptor = nil;
#else
    void* device = nullptr;
    void* currentPipelineState = nullptr;
    void* currentDepthStencilState = nullptr;
    void* blendDescriptor = nullptr;
#endif

    std::unordered_map<size_t, void*> pipelineStateCache;
};
