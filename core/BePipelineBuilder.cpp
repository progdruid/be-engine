#include "BePipelineBuilder.h"

#include "BeShader.h"

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

auto BePipelineBuilder::BuildCompute(const BeShader& shader) -> SenPipeline {
    return BeBackend::GetComputePipeline(shader.ShaderID, shader.GetPipelineDesc());
}

auto BePipelineBuilder::Build() const -> SenPipeline {
    return BeBackend::GetPipeline(_key, *_baseDesc);
}
