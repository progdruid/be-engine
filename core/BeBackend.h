#pragma once
#include <array>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeRenderer.h"
#include "sen-rhi/SenTypes.h"

struct BeShader;

class BeBackend {
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Init() -> void;
    expose static auto Shutdown() -> void;

    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // pipelines ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct StaticKey {
        uint32_t ShaderID;
        SenTopology Topology;
        SenRasterizerState RasterizerState;
        SenBlendState BlendState;
        SenDepthStencilState DepthStencilState;

        StaticKey() { std::memset(this, 0, sizeof(StaticKey)); }
        auto operator==(const StaticKey& other) const -> bool {
            return std::memcmp(this, &other, sizeof(StaticKey)) == 0;
        }
    };

    expose struct FormatSet {
        std::array<SenFormat, 8> ColorFormats;
        SenFormat DepthFormat;

        FormatSet() { std::memset(this, 0, sizeof(FormatSet)); }
        auto operator==(const FormatSet& other) const -> bool {
            return std::memcmp(this, &other, sizeof(FormatSet)) == 0;
        }
    };

    hide struct BytesHash {
        template <typename T>
        auto operator()(const T& value) const -> size_t {
            return std::hash<std::string_view>()(std::string_view(reinterpret_cast<const char*>(&value), sizeof(T)));
        }
    };

    hide static std::vector<StaticKey> _staticKeys;
    hide static std::unordered_map<StaticKey, uint32_t, BytesHash> _staticKeyLookup;
    hide static std::vector<FormatSet> _formatSets;
    hide static std::unordered_map<FormatSet, uint32_t, BytesHash> _formatSetLookup;
    hide static std::vector<std::vector<SenPipeline>> _pipelines;

    expose static auto AcquireStaticKeyId(const StaticKey& key) -> uint32_t;
    expose static auto GetStaticKey(uint32_t staticKeyId) -> const StaticKey&;
    expose static auto AcquireFormatSetId(const FormatSet& formatSet) -> uint32_t;
    expose static auto GetPipeline(const BeShader& shader, uint32_t staticKeyId, uint32_t formatSetId) -> SenPipeline;
    expose static auto RebuildPipelines(const BeShader& shader) -> uint32_t;
    hide static auto MakePipeline(const BeShader& shader, uint32_t staticKeyId, uint32_t formatSetId) -> SenPipeline;
    
    
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
