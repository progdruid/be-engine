#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <umbrellas/access-modifiers.hpp>

#include "RhiTypes.h"

class RhiBuffer;

class RhiContext {
    expose
    virtual ~RhiContext() = default;

    virtual auto SetViewport(const RhiViewport& viewport) -> void = 0;
    virtual auto SetTopology(RhiTopology topology) -> void = 0;

    virtual auto MapBuffer(const std::shared_ptr<RhiBuffer>& buffer, void** outData) -> void = 0;
    virtual auto UnmapBuffer(const std::shared_ptr<RhiBuffer>& buffer) -> void = 0;

    virtual auto Draw(uint32_t vertexCount, uint32_t startVertex) -> void = 0;
    virtual auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) -> void = 0;
};
