#pragma once
#ifdef __APPLE__

#include <memory>
#include <string>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiTypes.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalTexture;

struct MetalPipelineDesc {
    std::string VertexFunction;
    std::string FragmentFunction;
    RhiFormat ColorFormats[8] = {};
    uint32_t ColorFormatCount = 0;
    RhiFormat DepthFormat = RhiFormat::Unknown;
    bool BlendEnabled = false;
};

class MetalPipeline {

    hide
#ifdef __OBJC__
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLDepthStencilState> _depthStencilState;
    id<MTLLibrary> _library;
#else
    id _pipelineState;
    id _depthStencilState;
    id _library;
#endif

    expose
#ifdef __OBJC__
    static auto Create(
        id<MTLDevice> device,
        id<MTLLibrary> library,
        const MetalPipelineDesc& desc
    ) -> std::shared_ptr<MetalPipeline>;
#endif

    ~MetalPipeline();

#ifdef __OBJC__
    auto GetPipelineState() const -> id<MTLRenderPipelineState> { return _pipelineState; }
    auto GetDepthStencilState() const -> id<MTLDepthStencilState> { return _depthStencilState; }
#endif
};

#endif
