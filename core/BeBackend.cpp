#include "BeBackend.h"

#include <algorithm>
#include <ranges>
#include <string_view>

#include "sen-rhi/SenBackend.h"
#include <umbrellas/include-libassert.h>

std::unordered_map<BeBackend::PipelineKey, SenPipeline, BeBackend::PipelineKeyHash> BeBackend::_pipelines;
std::unordered_map<uint32_t, SenPipeline> BeBackend::_computePipelines;
std::array<BeBackend::MaterialArenaChain, BeRenderer::FramesInFlight> BeBackend::_arenaChains;

auto BeBackend::Init() -> void {}

auto BeBackend::Shutdown() -> void {
    for (const auto pipeline : _pipelines | std::views::values) {
        SenBackend::RetirePipeline(pipeline);
    }
    for (const auto pipeline : _computePipelines | std::views::values) {
        SenBackend::RetirePipeline(pipeline);
    }
    _pipelines.clear();
    _computePipelines.clear();

    for (auto& chain : _arenaChains) {
        for (const auto& block : chain.Blocks) {
            SenBackend::RetireBuffer(block.Buffer);
        }
        chain = {};
    }
}




auto BeBackend::PipelineKeyHash::operator()(const PipelineKey& key) const -> size_t {
    const auto bytes = std::string_view(reinterpret_cast<const char*>(&key), sizeof(key));
    return std::hash<std::string_view>()(bytes);
}

auto BeBackend::GetPipeline(const PipelineKey& key, const SenPipelineDesc& baseDesc) -> SenPipeline {
    const auto it = _pipelines.find(key);
    if (it != _pipelines.end()) {
        return it->second;
    }

    auto desc = baseDesc;
    desc.Topology = key.Topology;
    desc.RasterizerState = key.RasterizerState;
    desc.BlendState = key.BlendState;
    desc.DepthStencilState = key.DepthStencilState;
    desc.RenderTargetFormats.clear();
    for (const auto format : key.ColorFormats) {
        if (format == SenFormat::Unknown) {
            break;
        }
        desc.RenderTargetFormats.push_back(format);
    }
    desc.DepthStencilFormat = key.DepthFormat;

    const auto pipeline = SenBackend::CreatePipeline(desc);
    _pipelines[key] = pipeline;
    return pipeline;
}

auto BeBackend::GetComputePipeline(uint32_t shaderID, const SenPipelineDesc& desc) -> SenPipeline {
    const auto it = _computePipelines.find(shaderID);
    if (it != _computePipelines.end()) {
        return it->second;
    }

    const auto pipeline = SenBackend::CreatePipeline(desc);
    _computePipelines[shaderID] = pipeline;
    return pipeline;
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
