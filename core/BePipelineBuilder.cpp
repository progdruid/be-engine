#include "BePipelineBuilder.h"

#include "BeShader.h"
#include "sen-rhi/SenBackend.h"
#include <string_view>

std::unordered_map<
    BePipelineBuilder::CachedPipelineKey, 
    SenPipeline, 
    BePipelineBuilder::CachedPipelineHash
> BePipelineBuilder::_cachedPipelines;

auto BePipelineBuilder::CachedPipelineHash::operator()(const CachedPipelineKey& k) const -> size_t {
    auto hasher         = std::hash<std::string_view>();
    const auto* bytes   = reinterpret_cast<const char*>(&k);
    const auto view     = std::string_view(bytes, sizeof(k));
    return hasher(view);
}

auto BePipelineBuilder::Start(std::weak_ptr<BeShader> shader) -> BePipelineBuilder {
    auto shared = shader.lock();
    return BePipelineBuilder(shared->GetPipelineDesc(), shared->ShaderID);
}

BePipelineBuilder::BePipelineBuilder(SenPipelineDesc desc, uint32_t shaderID) : _desc(std::move(desc)) {
    _key.ShaderID = shaderID;
    _key.Topology = _desc.Topology;
    _key.RasterizerState = _desc.RasterizerState;
    _key.BlendState = _desc.BlendState;
    _key.DepthStencilState = _desc.DepthStencilState;
    for (size_t i = 0; i < _desc.RenderTargetFormats.size(); ++i) {
        _key.ColorFormats[i] = _desc.RenderTargetFormats[i];
    }
    _key.DepthFormat = _desc.DepthStencilFormat;
}

auto BePipelineBuilder::SetTopology(SenTopology topology) -> BePipelineBuilder& {
    _desc.Topology = topology; 
    _key.Topology = topology;
    return *this;
}

auto BePipelineBuilder::SetRasterizer(const SenRasterizerState& rasterizer) -> BePipelineBuilder& {
    _desc.RasterizerState = rasterizer;
    _key.RasterizerState = rasterizer;
    return *this;
}

auto BePipelineBuilder::SetBlend(SenBlendState blend) -> BePipelineBuilder& {
    _desc.BlendState = blend; 
    _key.BlendState = blend;
    return *this;
}

auto BePipelineBuilder::SetDepthStencil(const SenDepthStencilState& depthStencil) -> BePipelineBuilder& {
    _desc.DepthStencilState = depthStencil; 
    _key.DepthStencilState = depthStencil;
    return *this;
}

auto BePipelineBuilder::SetTargetFormats(const std::vector<SenFormat>& colorFormats, SenFormat depthFormat) -> BePipelineBuilder& {
    _desc.RenderTargetFormats = colorFormats;
    _desc.DepthStencilFormat = depthFormat;
    for (size_t i = 0; i < _desc.RenderTargetFormats.size(); ++i) {
        _key.ColorFormats[i] = _desc.RenderTargetFormats[i];
    }
    for (size_t i = _desc.RenderTargetFormats.size(); i < _key.ColorFormats.size(); ++i) {
        _key.ColorFormats[i] = SenFormat::Unknown;
    }
    _key.DepthFormat = depthFormat;
    return *this;
}

auto BePipelineBuilder::Build() const -> SenPipeline {
    auto it = _cachedPipelines.find(_key);
    if (it != _cachedPipelines.end()) {
        return it->second;
    } else {
        return (_cachedPipelines[_key] = SenBackend::CreatePipeline(_desc));
    }
}
