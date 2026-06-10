#pragma once
#include <utility>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>
#include <sen-rhi/SenCommandBuffer.h>

// Accumulates texture state transitions and flushes them as one barrier at a pass boundary.
class SenTransitionBatch {
    hide
    std::vector<std::pair<SenTexture, SenResourceState>> _pending;

    expose
    auto Add(SenTexture texture, SenResourceState state) -> void;
    auto TransitionAll(SenCommandBuffer& cmd) -> void;  // emits the barrier, then clears for reuse
};
