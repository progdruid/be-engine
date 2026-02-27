#pragma once
#ifdef __APPLE__

#include <memory>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiContext.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalBuffer;

class MetalContext final : public RhiContext {

    hide
#ifdef __OBJC__
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLCommandBuffer> _currentCommandBuffer;
    id<MTLRenderCommandEncoder> _currentEncoder;
#else
    id _device;
    id _commandQueue;
    id _currentCommandBuffer;
    id _currentEncoder;
#endif

    expose
#ifdef __OBJC__
    explicit MetalContext(id<MTLDevice> device);
#else
    explicit MetalContext(id device);
#endif
    ~MetalContext() override;

    auto SetViewport(const RhiViewport& viewport) -> void override;
    auto SetTopology(RhiTopology topology) -> void override;

    auto MapBuffer(const std::shared_ptr<RhiBuffer>& buffer, void** outData) -> void override;
    auto UnmapBuffer(const std::shared_ptr<RhiBuffer>& buffer) -> void override;

    auto Draw(uint32_t vertexCount, uint32_t startVertex) -> void override;
    auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) -> void override;

#ifdef __OBJC__
    auto BeginFrame() -> id<MTLCommandBuffer>;
    auto GetCommandQueue() const -> id<MTLCommandQueue> { return _commandQueue; }
    auto GetCurrentCommandBuffer() const -> id<MTLCommandBuffer> { return _currentCommandBuffer; }
    auto GetCurrentEncoder() const -> id<MTLRenderCommandEncoder> { return _currentEncoder; }

    auto BeginRenderPass(MTLRenderPassDescriptor* descriptor) -> void;
    auto EndRenderPass() -> void;
    auto CommitAndWait() -> void;
#endif
};

#endif
