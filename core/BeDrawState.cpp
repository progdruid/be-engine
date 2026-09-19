#include "BeDrawState.h"

#include <utility>

#include "BeShader.h"

BeDrawState::Builder::Builder(const BeShader& shader) : _shader(&shader) {
    _key.ShaderID = shader.ShaderID;
    _key.Topology = shader.Topology;
    _key.RasterizerState = shader.RasterizerState;
    _key.BlendState = shader.BlendState;
    _key.DepthStencilState = shader.DepthStencilState;
}

auto BeDrawState::Builder::SetTopology(SenTopology topology) -> Builder&& {
    _key.Topology = topology;
    return std::move(*this);
}

auto BeDrawState::Builder::SetCull(SenCullMode mode) -> Builder&& {
    _key.RasterizerState.CullMode = mode;
    return std::move(*this);
}

auto BeDrawState::Builder::SetFill(SenFillMode mode) -> Builder&& {
    _key.RasterizerState.FillMode = mode;
    return std::move(*this);
}

auto BeDrawState::Builder::SetRasterizer(const SenRasterizerState& rasterizer) -> Builder&& {
    _key.RasterizerState = rasterizer;
    return std::move(*this);
}

auto BeDrawState::Builder::SetBlend(const SenBlendState& blend) -> Builder&& {
    _key.BlendState = blend;
    return std::move(*this);
}

auto BeDrawState::Builder::SetDepthStencil(const SenDepthStencilState& depthStencil) -> Builder&& {
    _key.DepthStencilState = depthStencil;
    return std::move(*this);
}

auto BeDrawState::Builder::Build() -> BeDrawState {
    auto state = BeDrawState();
    state._shader = _shader;
    state._staticKeyId = BeBackend::AcquireStaticKeyId(_key);
    return state;
}
