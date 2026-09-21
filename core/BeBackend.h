#pragma once
#include <array>
#include <cstring>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeRenderer.h"
#include "sen-rhi/SenTypes.h"
#include <sen-rhi/Sen.h>

struct BeShader;
class BeTexture;

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
        SenFill Fill;
        bool DepthClipEnable;
        SenBlendState BlendState;

        StaticKey() { std::memset(this, 0, sizeof(StaticKey)); }
        auto operator==(const StaticKey& other) const -> bool {
            return std::memcmp(this, &other, sizeof(StaticKey)) == 0;
        }
    };

    expose struct DynamicState {
        SenCull Cull = SenCull::Back;
        SenFrontFace FrontFace = SenFrontFace::Clockwise;
        SenDepthState Depth;
        float DepthBias = 0.f;
        float SlopeScaledDepthBias = 0.f;
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
    // uploads /////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide static SenCommandList _uploadCmd;

    expose static auto WriteBuffer(const void* data, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void;
    expose static auto WriteTexture(const void* data, uint32_t size, SenTexture dst) -> void;
    expose static auto GenerateMips(const std::shared_ptr<BeTexture>& texture) -> void;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // bindless table //////////////////////////////////////////////////////////////////////////////////////////////////
    hide static constexpr uint32_t InvalidSlot = UINT32_MAX;

    hide enum class BindlessKind : uint8_t {
        Texture,
        Sampler,
        Count,
    };

    hide static std::array<uint32_t,              static_cast<size_t>(BindlessKind::Count)> _bindlessNext;
    hide static std::array<std::vector<uint32_t>, static_cast<size_t>(BindlessKind::Count)> _bindlessFree;
    hide static std::array<uint32_t,              static_cast<size_t>(BindlessKind::Count)> _bindlessCapacity;
    hide static std::vector<uint32_t> _textureSlots;     // by SenView::Index
    hide static std::vector<uint32_t> _samplerSlots;     // by SenSampler::Index

    expose static auto RegisterTexture (SenView view) -> void;
    expose static auto RegisterSampler (SenSampler sampler) -> void;
    expose static auto GetTextureSlot  (SenView view) -> uint32_t;
    expose static auto GetSamplerSlot  (SenSampler sampler) -> uint32_t;

    hide static auto UnregisterTexture (SenView view) -> void;
    hide static auto UnregisterSampler (SenSampler sampler) -> void;
    hide static auto AllocSlot   (BindlessKind kind) -> uint32_t;
    hide static auto ReleaseSlot (BindlessKind kind, uint32_t slot) -> void;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // retirement //////////////////////////////////////////////////////////////////////////////////////////////////////
    hide struct RetirementBucket {
        SenSubmission Submission;
        std::vector<SenTexture> Textures;
        std::vector<SenView> Views;
        std::vector<SenBuffer> Buffers;
        std::vector<SenSampler> Samplers;
        std::vector<SenPipeline> Pipelines;
    };

    hide static RetirementBucket _pending;
    hide static std::vector<RetirementBucket> _retired;

    expose static auto Retire(SenTexture handle) -> void;
    expose static auto Retire(SenView handle) -> void;
    expose static auto Retire(SenBuffer handle) -> void;
    expose static auto Retire(SenSampler handle) -> void;
    expose static auto Retire(SenPipeline handle) -> void;
    expose static auto StampRetirements(SenSubmission submission) -> void;
    expose static auto FlushRetirements() -> void;
    hide static auto DestroyBucket(const RetirementBucket& bucket) -> void;


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
