#pragma once
#include <cstdint>
#include <umbrellas/access-modifiers.hpp>

#include "GalTypes.h"

class IGalBuffer {
    expose
    virtual ~IGalBuffer() = default;

    virtual auto GetByteWidth() const -> uint32_t = 0;
    virtual auto GetUsage() const -> GalBufferUsage = 0;
    virtual auto GetBindFlags() const -> GalBindFlags = 0;
};
