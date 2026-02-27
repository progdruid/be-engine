#pragma once
#include <cstdint>
#include <umbrellas/access-modifiers.hpp>

#include "RhiTypes.h"

class RhiBuffer {
    expose
    virtual ~RhiBuffer() = default;

    virtual auto GetByteWidth() const -> uint32_t = 0;
    virtual auto GetUsage() const -> RhiBufferUsage = 0;
    virtual auto GetBindFlags() const -> RhiBindFlags = 0;
};
