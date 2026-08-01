#include "BeMaterialArena.h"

#include "BeMaterialScheme.h"
#include "sen-rhi/SenBackend.h"
#include <umbrellas/include-libassert.h>

std::unordered_map<std::string, std::unique_ptr<BeMaterialArena>> BeMaterialArena::_arenas;

auto BeMaterialArena::Acquire(const BeMaterialScheme& scheme) -> BeMaterialArena* {
    auto it = _arenas.find(scheme.Name);
    if (it != _arenas.end()) {
        return it->second.get();
    }
    auto [inserted, _] = _arenas.emplace(scheme.Name, std::make_unique<BeMaterialArena>(scheme));
    return inserted->second.get();
}

auto BeMaterialArena::ResetForFrame(uint64_t frame) -> void {
    const uint32_t slot = frame % BeRenderer::FramesInFlight;
    for (const auto& [_, arena] : _arenas) {
        arena->_chains[slot].CurrentBlock = 0;
        arena->_chains[slot].BumpIndex = 0;
    }
}

auto BeMaterialArena::DestroyAll() -> void {
    _arenas.clear();
}

BeMaterialArena::BeMaterialArena(const BeMaterialScheme& scheme) : _schemeName(scheme.Name) {
    be_assert(scheme.CbufferSize > 0, "BeMaterialArena: scheme has no cbuffer: " + scheme.Name);

    const uint32_t alignment = SenBackend::GetMinUniformBufferOffsetAlignment();
    _stride = (scheme.CbufferSize + alignment - 1) / alignment * alignment;
}

BeMaterialArena::~BeMaterialArena() {
    for (const auto& chain : _chains) {
        for (const auto block : chain.Blocks) {
            SenBackend::DestroyBuffer(block);
        }
    }
}

auto BeMaterialArena::Allocate(uint64_t frame) -> Chunk {
    auto& chain = _chains[frame % BeRenderer::FramesInFlight];

    if (chain.BumpIndex >= ChunksPerBlock) {
        ++chain.CurrentBlock;
        chain.BumpIndex = 0;
    }
    if (chain.CurrentBlock >= chain.Blocks.size()) {
        chain.Blocks.push_back(SenBackend::CreateBuffer({
            .Usage  = SenBufferUsage::Constant,
            .Access = SenBufferAccess::Dynamic,
            .Size   = _stride * ChunksPerBlock,
        }));
    }

    return { frame, chain.CurrentBlock, chain.BumpIndex++ };
}

auto BeMaterialArena::GetBuffer(Chunk chunk) const -> SenBuffer {
    be_assert(chunk.IsValid(), "BeMaterialArena: chunk was never allocated");
    return _chains[chunk.Frame % BeRenderer::FramesInFlight].Blocks[chunk.Block];
}

auto BeMaterialArena::GetOffset(Chunk chunk) const -> uint32_t {
    be_assert(chunk.IsValid(), "BeMaterialArena: chunk was never allocated");
    return chunk.Index * _stride;
}
