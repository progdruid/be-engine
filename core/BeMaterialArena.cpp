#include "BeMaterialArena.h"

#include <algorithm>

#include "sen-rhi/SenBackend.h"
#include <umbrellas/include-libassert.h>

std::array<BeMaterialArena::Chain, BeRenderer::FramesInFlight> BeMaterialArena::_chains;

auto BeMaterialArena::Allocate(uint64_t frame, uint32_t size) -> Chunk {
    be_assert(size > 0, "BeMaterialArena: empty allocation");
    const uint32_t alignedSize = (size + Alignment - 1) / Alignment * Alignment;
    auto& chain = _chains[frame % BeRenderer::FramesInFlight];

    while (chain.CurrentBlock < chain.Blocks.size() && chain.BumpOffset + alignedSize > chain.Blocks[chain.CurrentBlock].Size) {
        ++chain.CurrentBlock;
        chain.BumpOffset = 0;
    }
    if (chain.CurrentBlock == chain.Blocks.size()) {
        const uint32_t blockSize = std::max(DefaultBlockSize, alignedSize);
        const SenBuffer buffer = SenBackend::CreateBuffer({
            .Usage  = SenBufferUsage::Constant,
            .Access = SenBufferAccess::Dynamic,
            .Size   = blockSize,
        });
        chain.Blocks.push_back({ buffer, blockSize });
    }

    const Chunk chunk { frame, chain.Blocks[chain.CurrentBlock].Buffer, chain.BumpOffset };
    chain.BumpOffset += alignedSize;
    return chunk;
}

auto BeMaterialArena::ResetForFrame(uint64_t frame) -> void {
    auto& chain = _chains[frame % BeRenderer::FramesInFlight];
    chain.CurrentBlock = 0;
    chain.BumpOffset = 0;
}

auto BeMaterialArena::DestroyAll() -> void {
    for (auto& chain : _chains) {
        for (const auto& block : chain.Blocks) {
            SenBackend::RetireBuffer(block.Buffer);
        }
        chain = {};
    }
}
