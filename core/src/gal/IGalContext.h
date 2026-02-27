#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <umbrellas/access-modifiers.hpp>

#include "GalTypes.h"

class IGalBuffer;

class IGalContext {
    expose
    virtual ~IGalContext() = default;

    virtual auto SetViewport(const GalViewport& viewport) -> void = 0;
    virtual auto SetTopology(GalTopology topology) -> void = 0;

    virtual auto MapBuffer(const std::shared_ptr<IGalBuffer>& buffer, void** outData) -> void = 0;
    virtual auto UnmapBuffer(const std::shared_ptr<IGalBuffer>& buffer) -> void = 0;

    virtual auto Draw(uint32_t vertexCount, uint32_t startVertex) -> void = 0;
    virtual auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) -> void = 0;
};
