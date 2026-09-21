#include "BeBackend.h"

#include <algorithm>
#include <ranges>
#include <string_view>

#include "BeDrawState.h"
#include "BeMaterial.h"
#include "BePass.h"
#include "BeShader.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "sen-rhi/Sen.h"
#include <umbrellas/include-libassert.h>


std::vector<BeBackend::StaticKey> BeBackend::_staticKeys;
std::unordered_map<BeBackend::StaticKey, uint32_t, BeBackend::BytesHash> BeBackend::_staticKeyLookup;
std::vector<BeBackend::FormatSet> BeBackend::_formatSets;
std::unordered_map<BeBackend::FormatSet, uint32_t, BeBackend::BytesHash> BeBackend::_formatSetLookup;
std::vector<std::vector<SenPipeline>> BeBackend::_pipelines;
SenCommandList BeBackend::_uploadCmd;
std::array<uint32_t, static_cast<size_t>(BeBackend::BindlessKind::Count)> BeBackend::_bindlessNext {};
std::array<std::vector<uint32_t>, static_cast<size_t>(BeBackend::BindlessKind::Count)> BeBackend::_bindlessFree {};
std::array<uint32_t, static_cast<size_t>(BeBackend::BindlessKind::Count)> BeBackend::_bindlessCapacity {};
std::vector<uint32_t> BeBackend::_textureSlots;
std::vector<uint32_t> BeBackend::_samplerSlots;
BeBackend::RetirementBucket BeBackend::_pending;
std::vector<BeBackend::RetirementBucket> BeBackend::_retired;
std::array<BeBackend::MaterialArenaChain, BeRenderer::FramesInFlight> BeBackend::_arenaChains;


auto BeBackend::Init() -> void {
    _uploadCmd = Sen::CreateCommandList();

    const SenCaps caps = Sen::GetCaps();
    _bindlessCapacity[static_cast<size_t>(BindlessKind::Texture)] = caps.TextureSlots;
    _bindlessCapacity[static_cast<size_t>(BindlessKind::Sampler)] = caps.SamplerSlots;
}

auto BeBackend::Shutdown() -> void {
    Sen::WaitIdle();

    for (const auto& bucket : _retired) {
        DestroyBucket(bucket);
    }
    _retired.clear();
    DestroyBucket(_pending);
    _pending = {};

    for (const auto pipeline : _pipelines | std::views::join) {
        if (pipeline.IsValid()) {
            Sen::DestroyPipeline(pipeline);
        }
    }
    _pipelines.clear();
    _staticKeys.clear();
    _staticKeyLookup.clear();
    _formatSets.clear();
    _formatSetLookup.clear();

    for (auto& chain : _arenaChains) {
        for (const auto& block : chain.Blocks) {
            Sen::DestroyBuffer(block.Buffer);
        }
        chain = {};
    }

    _bindlessNext = {};
    for (auto& free : _bindlessFree) {
        free.clear();
    }
    _textureSlots.clear();
    _samplerSlots.clear();
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

    be_assert(
        _staticKeys.at(staticKeyId).ShaderID == shader.ShaderID,
        "BeBackend::GetPipeline: static key belongs to another shader"
    );

    row[formatSetId] = MakePipeline(shader, staticKeyId, formatSetId);
    return row[formatSetId];
}

auto BeBackend::RebuildPipelines(const BeShader& shader) -> uint32_t {
    auto count = uint32_t(0);
    for (uint32_t staticKeyId = 0; staticKeyId < _staticKeys.size(); ++staticKeyId) {
        if (_staticKeys[staticKeyId].ShaderID != shader.ShaderID) {
            continue;
        }
        auto& row = _pipelines[staticKeyId];
        for (uint32_t formatSetId = 0; formatSetId < row.size(); ++formatSetId) {
            if (!row[formatSetId].IsValid()) {
                continue;
            }
            Retire(row[formatSetId]);
            row[formatSetId] = MakePipeline(shader, staticKeyId, formatSetId);
            ++count;
        }
    }
    return count;
}

auto BeBackend::MakePipeline(const BeShader& shader, uint32_t staticKeyId, uint32_t formatSetId) -> SenPipeline {
    const auto& key = _staticKeys.at(staticKeyId);
    const auto& formatSet = _formatSets.at(formatSetId);

    auto desc = SenPipelineDesc();
    if (HasAny(shader.ShaderType, BeShaderType::Compute)) {
        desc.ComputeShader = shader.StageCompute.GetCode();
    }
    else {
        desc.VertexShader = shader.StageVertex.GetCode();
        desc.HullShader = shader.StageHull.GetCode();
        desc.DomainShader = shader.StageDomain.GetCode();
        desc.PixelShader = shader.StagePixel.GetCode();
        desc.VertexLayout = shader.VertexLayout;
        desc.VertexStride = shader.VertexStride;
        desc.Topology = key.Topology;
        desc.Fill = key.Fill;
        desc.DepthClipEnable = key.DepthClipEnable;
        desc.BlendState = key.BlendState;
        for (const auto format : formatSet.ColorFormats) {
            if (format == SenFormat::Unknown) {
                break;
            }
            desc.RenderTargetFormats.push_back(format);
        }
        desc.DepthFormat = formatSet.DepthFormat;
    }

    return Sen::CreatePipeline(desc);
}




auto BeBackend::WriteBuffer(const void* data, uint32_t size, SenBuffer dst, uint32_t dstOffset) -> void {
    if (Sen::GetBufferMemory(dst) == SenMemory::Upload) {
        auto* pointer = static_cast<uint8_t*>(Sen::GetBufferPointer(dst)) + dstOffset;
        std::memcpy(pointer, data, size);
        return;
    }

    const SenBuffer staging = Sen::CreateBuffer({
        .Memory = SenMemory::Upload,
        .Size = size,
    });
    std::memcpy(Sen::GetBufferPointer(staging), data, size);

    SenCmd::Begin(_uploadCmd);
    SenCmd::CopyBuffer(_uploadCmd, staging, 0, size, dst, dstOffset);
    SenCmd::End(_uploadCmd);

    const SenSubmission submission = Sen::Submit({ .Lists = &_uploadCmd, .ListCount = 1 });
    Sen::WaitForSubmission(submission);
    Retire(staging);
    StampRetirements(submission);
}

auto BeBackend::WriteTexture(const void* data, uint32_t size, SenTexture dst) -> void {
    const SenBuffer staging = Sen::CreateBuffer({
        .Memory = SenMemory::Upload,
        .Size = size,
    });
    std::memcpy(Sen::GetBufferPointer(staging), data, size);

    SenCmd::Begin(_uploadCmd);
    SenCmd::TransitionTextures(_uploadCmd, { { dst, SenLayout::TransferDst } });
    SenCmd::CopyBufferToTexture(_uploadCmd, staging, 0, dst, 0);
    SenCmd::TransitionTextures(_uploadCmd, { { dst, SenLayout::ShaderRead } });
    SenCmd::End(_uploadCmd);

    const SenSubmission submission = Sen::Submit({ .Lists = &_uploadCmd, .ListCount = 1 });
    Sen::WaitForSubmission(submission);
    Retire(staging);
    StampRetirements(submission);
}

auto BeBackend::GenerateMips(const std::shared_ptr<BeTexture>& texture) -> void {
    be_assert(texture->Mips > 1, "BeBackend::GenerateMips: texture has only one mip level");

    const auto shader = BeShaderLibrary::GetShader("mip-downsample");
    const auto state = BeDrawState::Create(*shader).Build();
    const auto& scheme = BeShaderLibrary::GetShaderScheme(*shader, "main");

    SenCmd::Begin(_uploadCmd);

    for (uint32_t mip = 1; mip < texture->Mips; ++mip) {
        const auto& source = texture->GetMipViewport(mip - 1);
        const auto material = BeMaterial::Create(scheme);
        material->SetFloat2("TexelSize", glm::vec2(1.f / source.Width, 1.f / source.Height));
        material->SetTexture("SourceMip", texture, mip - 1);

        BePass pass(_uploadCmd);
        pass.UseTextureMip(texture, mip - 1);
        pass.AddColorTarget(texture, SenLoadOp::DontCare, {}, mip);
        pass.SetViewport(texture->GetMipViewport(mip));
        pass.Begin();
        pass.SetState(state);
        pass.Bind("main", *material);
        pass.Draw(4);
        pass.End();
    }

    SenCmd::TransitionTextures(_uploadCmd, { { texture->Handle, SenLayout::ShaderRead } });
    SenCmd::End(_uploadCmd);

    const SenSubmission submission = Sen::Submit({ .Lists = &_uploadCmd, .ListCount = 1 });
    Sen::WaitForSubmission(submission);
    StampRetirements(submission);
}



auto BeBackend::RegisterTexture(SenView view) -> void {
    be_assert(view.IsValid(), "BeBackend::RegisterTexture: invalid view");

    if (view.Index >= _textureSlots.size()) {
        _textureSlots.resize(view.Index + 1, InvalidSlot);
    }
    be_assert(_textureSlots[view.Index] == InvalidSlot, "BeBackend::RegisterTexture: view already registered", view.Index);

    const uint32_t slot = AllocSlot(BindlessKind::Texture);
    _textureSlots[view.Index] = slot;
    Sen::PublishTextureBindless(slot, view);
}

auto BeBackend::RegisterSampler(SenSampler sampler) -> void {
    be_assert(sampler.IsValid(), "BeBackend::RegisterSampler: invalid sampler");

    if (sampler.Index >= _samplerSlots.size()) {
        _samplerSlots.resize(sampler.Index + 1, InvalidSlot);
    }
    be_assert(_samplerSlots[sampler.Index] == InvalidSlot, "BeBackend::RegisterSampler: sampler already registered", sampler.Index);

    const uint32_t slot = AllocSlot(BindlessKind::Sampler);
    _samplerSlots[sampler.Index] = slot;
    Sen::PublishSamplerBindless(slot, sampler);
}

auto BeBackend::GetTextureSlot(SenView view) -> uint32_t {
    be_assert(view.Index < _textureSlots.size(), "BeBackend::GetTextureSlot: view not registered", view.Index);
    const uint32_t slot = _textureSlots[view.Index];
    be_assert(slot != InvalidSlot, "BeBackend::GetTextureSlot: view not registered", view.Index);
    return slot;
}

auto BeBackend::GetSamplerSlot(SenSampler sampler) -> uint32_t {
    be_assert(sampler.Index < _samplerSlots.size(), "BeBackend::GetSamplerSlot: sampler not registered", sampler.Index);
    const uint32_t slot = _samplerSlots[sampler.Index];
    be_assert(slot != InvalidSlot, "BeBackend::GetSamplerSlot: sampler not registered", sampler.Index);
    return slot;
}

auto BeBackend::UnregisterTexture(SenView view) -> void {
    if (view.Index >= _textureSlots.size() || _textureSlots[view.Index] == InvalidSlot) {
        return;
    }
    ReleaseSlot(BindlessKind::Texture, _textureSlots[view.Index]);
    _textureSlots[view.Index] = InvalidSlot;
}

auto BeBackend::UnregisterSampler(SenSampler sampler) -> void {
    if (sampler.Index >= _samplerSlots.size() || _samplerSlots[sampler.Index] == InvalidSlot) {
        return;
    }
    ReleaseSlot(BindlessKind::Sampler, _samplerSlots[sampler.Index]);
    _samplerSlots[sampler.Index] = InvalidSlot;
}

auto BeBackend::AllocSlot(BindlessKind kind) -> uint32_t {
    auto& free = _bindlessFree[static_cast<size_t>(kind)];
    if (!free.empty()) {
        const uint32_t recycled = free.back();
        free.pop_back();
        return recycled;
    }

    const uint32_t slot = _bindlessNext[static_cast<size_t>(kind)]++;
    be_assert(
        slot < _bindlessCapacity[static_cast<size_t>(kind)],
        "BeBackend: out of bindless slots",
        static_cast<size_t>(kind)
    );
    return slot;
}

auto BeBackend::ReleaseSlot(BindlessKind kind, uint32_t slot) -> void {
    _bindlessFree[static_cast<size_t>(kind)].push_back(slot);
}



auto BeBackend::Retire(SenTexture handle)  -> void { _pending.Textures.push_back(handle); }
auto BeBackend::Retire(SenView handle)     -> void { _pending.Views.push_back(handle); }
auto BeBackend::Retire(SenBuffer handle)   -> void { _pending.Buffers.push_back(handle); }
auto BeBackend::Retire(SenSampler handle)  -> void { _pending.Samplers.push_back(handle); }
auto BeBackend::Retire(SenPipeline handle) -> void { _pending.Pipelines.push_back(handle); }

auto BeBackend::StampRetirements(SenSubmission submission) -> void {
    _pending.Submission = submission;
    _retired.push_back(std::move(_pending));
    _pending = {};
}

auto BeBackend::FlushRetirements() -> void {
    size_t flushed = 0;
    while (flushed < _retired.size() && Sen::IsSubmissionComplete(_retired[flushed].Submission)) {
        DestroyBucket(_retired[flushed]);
        ++flushed;
    }
    _retired.erase(_retired.begin(), _retired.begin() + flushed);
}

auto BeBackend::DestroyBucket(const RetirementBucket& bucket) -> void {
    for (const auto handle : bucket.Views)     { UnregisterTexture(handle); Sen::DestroyView(handle); }
    for (const auto handle : bucket.Textures)  {                            Sen::DestroyTexture(handle); }
    for (const auto handle : bucket.Buffers)   {                            Sen::DestroyBuffer(handle); }
    for (const auto handle : bucket.Samplers)  { UnregisterSampler(handle); Sen::DestroySampler(handle); }
    for (const auto handle : bucket.Pipelines) {                            Sen::DestroyPipeline(handle); }
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
        const SenBuffer buffer = Sen::CreateBuffer({
            .Memory = SenMemory::Upload,
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
