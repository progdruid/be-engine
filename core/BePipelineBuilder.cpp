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
    auto hash = hasher(view);
    return hash;
}

auto BePipelineBuilder::Start(const BeShader& shader) -> BePipelineBuilder {
    const auto& desc = shader.GetPipelineDesc();
    BePipelineBuilder builder(desc, shader.ShaderID);
    return builder;
}

BePipelineBuilder::BePipelineBuilder(const SenPipelineDesc& desc, uint32_t shaderID) : _baseDesc(&desc) {
    _key.ShaderID = shaderID;
    _key.Topology = desc.Topology;
    _key.RasterizerState = desc.RasterizerState;
    _key.BlendState = desc.BlendState;
    _key.DepthStencilState = desc.DepthStencilState;
    for (size_t i = 0; i < desc.RenderTargetFormats.size(); ++i) {
        _key.ColorFormats[i] = desc.RenderTargetFormats[i];
    }
    _key.DepthFormat = desc.DepthStencilFormat;
}

BePipelineBuilder::~BePipelineBuilder() = default;

auto BePipelineBuilder::SetTopology(SenTopology topology) -> BePipelineBuilder& {
    _key.Topology = topology;
    return *this;
}

auto BePipelineBuilder::SetCullMode(SenCullMode mode) -> BePipelineBuilder& {
    _key.RasterizerState.CullMode = mode;
    return *this;
}

auto BePipelineBuilder::SetFillMode(SenFillMode mode) -> BePipelineBuilder& {
    _key.RasterizerState.FillMode = mode;
    return *this;
}

auto BePipelineBuilder::SetRasterizer(const SenRasterizerState& rasterizer) -> BePipelineBuilder& {
    _key.RasterizerState = rasterizer;
    return *this;
}

auto BePipelineBuilder::SetBlend(SenBlendState blend) -> BePipelineBuilder& {
    _key.BlendState = blend;
    return *this;
}

auto BePipelineBuilder::SetDepthStencil(const SenDepthStencilState& depthStencil) -> BePipelineBuilder& {
    _key.DepthStencilState = depthStencil;
    return *this;
}

auto BePipelineBuilder::SetColorFormats(std::initializer_list<SenFormat> colorFormats) -> BePipelineBuilder& {
    size_t i = 0;
    for (auto format : colorFormats) {
        _key.ColorFormats[i++] = format;
    }
    for (size_t i = colorFormats.size(); i < _key.ColorFormats.size(); ++i) {
        _key.ColorFormats[i] = SenFormat::Unknown;
    }
    return *this;
}

auto BePipelineBuilder::SetColorFormats(std::vector<SenFormat> colorFormats) -> BePipelineBuilder& {
    size_t i = 0;
    for (auto format : colorFormats) {
        _key.ColorFormats[i++] = format;
    }
    for (size_t i = colorFormats.size(); i < _key.ColorFormats.size(); ++i) {
        _key.ColorFormats[i] = SenFormat::Unknown;
    }
    return *this;
}

auto BePipelineBuilder::SetDepthFormat(SenFormat depthFormat) -> BePipelineBuilder& {
    _key.DepthFormat = depthFormat;
    return *this;
}

auto BePipelineBuilder::Build() const -> SenPipeline {
    auto it = _cachedPipelines.find(_key);
    if (it != _cachedPipelines.end()) {
        return it->second;
    } else {
        SenPipelineDesc desc = *_baseDesc;
        desc.Topology = _key.Topology;
        desc.RasterizerState = _key.RasterizerState;
        desc.BlendState = _key.BlendState;
        desc.DepthStencilState = _key.DepthStencilState;
        desc.RenderTargetFormats.clear();
        for (auto fmt : _key.ColorFormats) {
            if (fmt == SenFormat::Unknown) break;
            desc.RenderTargetFormats.push_back(fmt);
        }
        desc.DepthStencilFormat = _key.DepthFormat;

        auto pipeline = SenBackend::CreatePipeline(desc);
        _cachedPipelines[_key] = pipeline;
        return pipeline;
    }
}
