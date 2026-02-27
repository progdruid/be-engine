#pragma once
#ifdef __APPLE__

#include <umbrellas/access-modifiers.hpp>

#include "../RhiBuffer.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

class MetalBuffer final : public RhiBuffer {

    hide
#ifdef __OBJC__
    id<MTLBuffer> _buffer;
#else
    id _buffer;
#endif
    uint32_t _byteWidth;
    RhiBufferUsage _usage;
    RhiBindFlags _bindFlags;

    expose
#ifdef __OBJC__
    explicit MetalBuffer(id<MTLBuffer> buffer, uint32_t byteWidth, RhiBufferUsage usage, RhiBindFlags bindFlags)
#else
    explicit MetalBuffer(id buffer, uint32_t byteWidth, RhiBufferUsage usage, RhiBindFlags bindFlags)
#endif
        : _buffer(buffer)
        , _byteWidth(byteWidth)
        , _usage(usage)
        , _bindFlags(bindFlags)
    {}

    ~MetalBuffer() override = default;

    auto GetByteWidth() const -> uint32_t override { return _byteWidth; }
    auto GetUsage() const -> RhiBufferUsage override { return _usage; }
    auto GetBindFlags() const -> RhiBindFlags override { return _bindFlags; }

#ifdef __OBJC__
    auto GetNative() const -> id<MTLBuffer> { return _buffer; }
#endif
    auto GetContents() const -> void* {
#ifdef __OBJC__
        return [_buffer contents];
#else
        return nullptr;
#endif
    }
};

#endif
