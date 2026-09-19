#include "BeBackend.h"

#include <algorithm>
#include <ranges>
#include <string_view>

#include "BeShader.h"
#include "sen-rhi/SenBackend.h"
#include <umbrellas/include-libassert.h>

std::vector<BeBackend::StaticKey> BeBackend::_staticKeys;
std::unordered_map<BeBackend::StaticKey, uint32_t, BeBackend::BytesHash> BeBackend::_staticKeyLookup;
std::vector<BeBackend::FormatSet> BeBackend::_formatSets;
std::unordered_map<BeBackend::FormatSet, uint32_t, BeBackend::BytesHash> BeBackend::_formatSetLookup;
std::vector<std::vector<SenPipeline>> BeBackend::_pipelines;
std::array<BeBackend::MaterialArenaChain, BeRenderer::FramesInFlight> BeBackend::_arenaChains;

auto BeBackend::Init() -> void {}

auto BeBackend::Shutdown() -> void {
    for (const auto pipeline : _pipelines | std::views::join) {
        if (pipeline.IsValid()) {
            SenBackend::RetirePipeline(pipeline);
        }
    }
    _pipelines.clear();
    _staticKeys.clear();
    _staticKeyLookup.clear();
    _formatSets.clear();
    _formatSetLookup.clear();

    for (auto& chain : _arenaChains) {
        for (const auto& block : chain.Blocks) {
            SenBackend::RetireBuffer(block.Buffer);
        }
        chain = {};
    }
}




auto BeBackend::AcquireStaticKeyId(const StaticKey& key) -> uint32_t {
    const auto it = _staticKeyLookup.find(key);
    if (it != _staticKeyLookup.end()) {
        return it->second;
    }
    const auto id = static_cast<uint32_t>(_staticKeys.size());
    _staticKeys.push_back(key);
    _staticKeyLookup.emplace(key, id);
    _pipelines.emplace_back();
    return id;
}

auto BeBackend::GetStaticKey(uint32_t staticKeyId) -> const StaticKey& {
    return _staticKeys.at(staticKeyId);
}

auto BeBackend::AcquireFormatSetId(const FormatSet& formatSet) -> uint32_t {
    const auto it = _formatSetLookup.find(formatSet);
    if (it != _formatSetLookup.end()) {
        return it->second;
    }
    const auto id = static_cast<uint32_t>(_formatSets.size());
    _formatSets.push_back(formatSet);
    _formatSetLookup.emplace(formatSet, id);
    return id;
}

auto BeBackend::GetPipeline(const BeShader& shader, uint32_t staticKeyId, uint32_t formatSetId) -> SenPipeline {
    auto& row = _pipelines[staticKeyId];
    if (formatSetId < row.size() && row[formatSetId].IsValid()) {
        return row[formatSetId];
    }
    if (formatSetId >= row.size()) {
        row.resize(formatSetId + 1);
    }

    const auto& key = _staticKeys.at(staticKeyId);
    const auto& formatSet = _formatSets.at(formatSetId);
    be_assert(key.ShaderID == shader.ShaderID, "BeBackend::GetPipeline: static key belongs to another shader");

    auto desc = SenPipelineDesc();
    if (HasAny(shader.ShaderType, BeShaderType::Compute)) {
        desc.ComputeShader = shader.ShaderCompute;
    }
    else {
        desc.VertexShader = shader.ShaderVertex;
        desc.HullShader = shader.ShaderHull;
        desc.DomainShader = shader.ShaderDomain;
        desc.PixelShader = shader.ShaderPixel;
        desc.VertexLayout = shader.VertexLayout;
        desc.VertexStride = shader.VertexStride;
        desc.Topology = key.Topology;
        desc.RasterizerState = key.RasterizerState;
        desc.BlendState = key.BlendState;
        desc.DepthStencilState = key.DepthStencilState;
        for (const auto format : formatSet.ColorFormats) {
            if (format == SenFormat::Unknown) {
                break;
            }
            desc.RenderTargetFormats.push_back(format);
        }
        desc.DepthStencilFormat = formatSet.DepthFormat;
    }

    row[formatSetId] = SenBackend::CreatePipeline(desc);
    return row[formatSetId];
}




auto BeBackend::AllocateMaterialArenaChunk(uint64_t frame, uint32_t size) -> MaterialArenaChunk {
    be_assert(size > 0, "BeBackend: empty material arena allocation");
    const uint32_t alignedSize = (size + ArenaAlignment - 1) / ArenaAlignment * ArenaAlignment;
    auto& chain = _arenaChains[frame % BeRenderer::FramesInFlight];

    while (chain.CurrentBlock < chain.Blocks.size() && chain.BumpOffset + alignedSize > chain.Blocks[chain.CurrentBlock].Size) {
        ++chain.CurrentBlock;
        chain.BumpOffset = 0;
    }
    if (chain.CurrentBlock == chain.Blocks.size()) {
        const uint32_t blockSize = std::max(ArenaBlockSize, alignedSize);
        const SenBuffer buffer = SenBackend::CreateBuffer({
            .Usage = SenBufferUsage::Constant,
            .Access = SenBufferAccess::Dynamic,
            .Size = blockSize,
        });
        chain.Blocks.push_back({ buffer, blockSize });
    }

    const MaterialArenaChunk chunk { frame, chain.Blocks[chain.CurrentBlock].Buffer, chain.BumpOffset };
    chain.BumpOffset += alignedSize;
    return chunk;
}

auto BeBackend::ResetMaterialArena(uint64_t frame) -> void {
    auto& chain = _arenaChains[frame % BeRenderer::FramesInFlight];
    chain.CurrentBlock = 0;
    chain.BumpOffset = 0;
}
