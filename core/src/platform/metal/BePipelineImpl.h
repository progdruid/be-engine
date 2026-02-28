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
    MTLRenderPipelineColorAttachmentDescriptor* blendDescriptor = nil;
    id<MTLBuffer> currentVertexBuffer = nil;
    size_t currentVertexBufferLength = 0;
    id<MTLBuffer> currentIndexBuffer = nil;
    size_t currentIndexBufferLength = 0;
#else
    void* device = nullptr;
    void* currentPipelineState = nullptr;
    void* currentDepthStencilState = nullptr;
    void* blendDescriptor = nullptr;
    void* currentVertexBuffer = nullptr;
    size_t currentVertexBufferLength = 0;
    void* currentIndexBuffer = nullptr;
    size_t currentIndexBufferLength = 0;
#endif

    std::unordered_map<size_t, void*> pipelineStateCache;
};
