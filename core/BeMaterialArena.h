#pragma once
#include <array>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeRenderer.h"
#include "sen-rhi/SenTypes.h"

// A bump arena of cbuffer chunks shared by all materials, one chain per frame in flight.
// Chains grow by appending blocks and are never resized or freed while the engine runs.
class BeMaterialArena {
    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    struct Chunk {
        uint64_t Frame = UINT64_MAX;
        SenBuffer Buffer;
        uint32_t Offset = 0;

        auto IsValid () const -> bool { return Frame != UINT64_MAX; }
    };

    hide
    struct Block {
        SenBuffer Buffer;
        uint32_t Size = 0;
    };

    struct Chain {
        std::vector<Block> Blocks;
        uint32_t CurrentBlock = 0;
        uint32_t BumpOffset = 0;
    };

    static constexpr uint32_t Alignment = 16;
    static constexpr uint32_t DefaultBlockSize = 256 * 1024;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    static std::array<Chain, BeRenderer::FramesInFlight> _chains;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Allocate (uint64_t frame, uint32_t size) -> Chunk;
    static auto ResetForFrame (uint64_t frame) -> void;
    static auto DestroyAll () -> void;
};
