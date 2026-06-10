#include "SenTransitionBatch.h"
#include <umbrellas/include-libassert.h>

auto SenTransitionBatch::Add(SenTexture texture, SenResourceState state) -> void {
    if (!texture.IsValid()) { return; }
    for (const auto& [tex, st] : _pending) {
        if (tex.ID == texture.ID) {
            be_assert(st == state, "SenTransitionBatch: texture added twice with conflicting states");
            return;
        }
    }
    _pending.emplace_back(texture, state);
}

auto SenTransitionBatch::TransitionAll(SenCommandBuffer& cmd) -> void {
    cmd.TransitionTextures(_pending);
    _pending.clear();
}
