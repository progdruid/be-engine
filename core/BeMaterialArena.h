#pragma once
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeRenderer.h"
#include "sen-rhi/SenTypes.h"

class BeMaterialScheme;

// A ring of fixed-stride cbuffer chunks for one material scheme, one chain per frame in flight.
// Chains grow by appending blocks and are never resized or freed while the engine runs.
class BeMaterialArena {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Acquire (const BeMaterialScheme& scheme) -> BeMaterialArena*;
    static auto ResetForFrame (uint64_t frame) -> void;
    static auto DestroyAll () -> void;

    hide
    static std::unordered_map<std::string, std::unique_ptr<BeMaterialArena>> _arenas;

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    struct Chunk {
        uint64_t Frame = UINT64_MAX;
        uint32_t Block = 0;
        uint32_t Index = 0;

        auto IsValid () const -> bool { return Frame != UINT64_MAX; }
    };

    hide
    struct Chain {
        std::vector<SenBuffer> Blocks;
        uint32_t CurrentBlock = 0;
        uint32_t BumpIndex = 0;
    };

    static constexpr uint32_t ChunksPerBlock = 256;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    std::string _schemeName;
    uint32_t _stride = 0;
    std::array<Chain, BeRenderer::FramesInFlight> _chains;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    explicit BeMaterialArena(const BeMaterialScheme& scheme);
    ~BeMaterialArena();

    BeMaterialArena(const BeMaterialArena&) = delete;
    BeMaterialArena& operator=(const BeMaterialArena&) = delete;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto Allocate (uint64_t frame) -> Chunk;

    auto GetBuffer (Chunk chunk) const -> SenBuffer;
    auto GetOffset (Chunk chunk) const -> uint32_t;
    auto GetStride () const -> uint32_t { return _stride; }
};
