#ifdef __APPLE__

#import "MetalContext.h"
#import "MetalBuffer.h"
#import <Metal/Metal.h>

MetalContext::MetalContext(id<MTLDevice> device)
    : _device(device)
    , _currentCommandBuffer(nil)
    , _currentEncoder(nil)
{
    _commandQueue = [device newCommandQueue];
}

MetalContext::~MetalContext() {
    _currentEncoder = nil;
    _currentCommandBuffer = nil;
    _commandQueue = nil;
}

auto MetalContext::SetViewport(const RhiViewport& viewport) -> void {
    if (!_currentEncoder) return;

    MTLViewport mtlViewport;
    mtlViewport.originX = viewport.X;
    mtlViewport.originY = viewport.Y;
    mtlViewport.width = viewport.Width;
    mtlViewport.height = viewport.Height;
    mtlViewport.znear = viewport.MinDepth;
    mtlViewport.zfar = viewport.MaxDepth;

    [_currentEncoder setViewport:mtlViewport];
}

auto MetalContext::SetTopology(RhiTopology topology) -> void {
    // Metal sets topology at draw time, stored for later
}

auto MetalContext::MapBuffer(const std::shared_ptr<RhiBuffer>& buffer, void** outData) -> void {
    auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
    *outData = metalBuffer->GetContents();
}

auto MetalContext::UnmapBuffer(const std::shared_ptr<RhiBuffer>& buffer) -> void {
    // Metal shared buffers are persistently mapped, no unmap needed
#ifdef __OBJC__
    auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
    auto native = metalBuffer->GetNative();
    if (native) {
        [native didModifyRange:NSMakeRange(0, metalBuffer->GetByteWidth())];
    }
#endif
}

auto MetalContext::Draw(uint32_t vertexCount, uint32_t startVertex) -> void {
    if (!_currentEncoder) return;
    [_currentEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:startVertex
                        vertexCount:vertexCount];
}

auto MetalContext::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) -> void {
    if (!_currentEncoder) return;
    // Note: Metal doesn't have baseVertex in drawIndexedPrimitives directly
    // for the basic version; use the offset variant
    // This will be refined when integrating with BeModel's shared buffers
}

auto MetalContext::BeginFrame() -> id<MTLCommandBuffer> {
    _currentCommandBuffer = [_commandQueue commandBuffer];
    return _currentCommandBuffer;
}

auto MetalContext::BeginRenderPass(MTLRenderPassDescriptor* descriptor) -> void {
    if (_currentEncoder) {
        [_currentEncoder endEncoding];
    }
    _currentEncoder = [_currentCommandBuffer renderCommandEncoderWithDescriptor:descriptor];
}

auto MetalContext::EndRenderPass() -> void {
    if (_currentEncoder) {
        [_currentEncoder endEncoding];
        _currentEncoder = nil;
    }
}

auto MetalContext::CommitAndWait() -> void {
    if (_currentCommandBuffer) {
        [_currentCommandBuffer commit];
        [_currentCommandBuffer waitUntilCompleted];
        _currentCommandBuffer = nil;
    }
}

#endif
