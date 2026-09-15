#pragma once
#include <array>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeRenderer.h"
#include "sen-rhi/SenTypes.h"

class BeBackend {
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Init() -> void;
    expose static auto Shutdown() -> void;

    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // pipelines ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct PipelineKey {
        uint32_t ShaderID;
        SenTopology Topology;
        SenRasterizerState RasterizerState;
        SenBlendState BlendState;
        SenDepthStencilState DepthStencilState;
        std::array<SenFormat, 8> ColorFormats;
        SenFormat DepthFormat;

        PipelineKey() { std::memset(this, 0, sizeof(PipelineKey)); }
        auto operator==(const PipelineKey& other) const -> bool {
            return std::memcmp(this, &other, sizeof(PipelineKey)) == 0;
        }
    };
    
    hide struct PipelineKeyHash {
        auto operator()(const PipelineKey& key) const -> size_t;
    };
    
    hide static std::unordered_map<PipelineKey, SenPipeline, PipelineKeyHash> _pipelines;
    hide static std::unordered_map<uint32_t, SenPipeline> _computePipelines;
    
    expose static auto GetPipeline(const PipelineKey& key, const SenPipelineDesc& baseDesc) -> SenPipeline;
    expose static auto GetComputePipeline(uint32_t shaderID, const SenPipelineDesc& desc) -> SenPipeline;
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // material arenas /////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct MaterialArenaChunk {
        uint64_t Frame = UINT64_MAX;
        SenBuffer Buffer;
        uint32_t Offset = 0;

        auto IsValid() const -> bool { return Frame != UINT64_MAX; }
    };
    
    hide struct MaterialArenaBlock {
        SenBuffer Buffer;
        uint32_t Size = 0;
    };
    hide struct MaterialArenaChain {
        std::vector<MaterialArenaBlock> Blocks;
        uint32_t CurrentBlock = 0;
        uint32_t BumpOffset = 0;
    };

    hide static constexpr uint32_t ArenaAlignment = 16;
    hide static constexpr uint32_t ArenaBlockSize = 256 * 1024;
    hide static std::array<MaterialArenaChain, BeRenderer::FramesInFlight> _arenaChains;
    
    expose static auto AllocateMaterialArenaChunk(uint64_t frame, uint32_t size) -> MaterialArenaChunk;
    expose static auto ResetMaterialArena(uint64_t frame) -> void;
};
