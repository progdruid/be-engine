#include "BeDrawState.h"

#include <utility>

#include "BeShader.h"

BeDrawState::Builder::Builder(const BeShader& shader) : _shader(&shader) {
    _key.ShaderID = shader.ShaderID;
    _key.Topology = shader.Topology;
    _key.Fill = shader.Fill;
    _key.DepthClipEnable = shader.DepthClipEnable;
    _key.BlendState = shader.BlendState;

    _dynamic.Cull = shader.Cull;
    _dynamic.FrontFace = shader.FrontFace;
    _dynamic.Depth = shader.DepthState;
}

auto BeDrawState::Builder::SetTopology(SenTopology topology) -> Builder&& {
    _key.Topology = topology;
    return std::move(*this);
}

auto BeDrawState::Builder::SetCull(SenCull mode) -> Builder&& {
    _dynamic.Cull = mode;
    return std::move(*this);
}

auto BeDrawState::Builder::SetFrontFace(SenFrontFace frontFace) -> Builder&& {
    _dynamic.FrontFace = frontFace;
    return std::move(*this);
}

auto BeDrawState::Builder::SetFill(SenFill mode) -> Builder&& {
    _key.Fill = mode;
    return std::move(*this);
}

auto BeDrawState::Builder::SetDepthClip(bool enable) -> Builder&& {
    _key.DepthClipEnable = enable;
    return std::move(*this);
}

auto BeDrawState::Builder::SetBlend(const SenBlendState& blend) -> Builder&& {
    _key.BlendState = blend;
    return std::move(*this);
}

auto BeDrawState::Builder::SetDepth(const SenDepthState& depthStencil) -> Builder&& {
    _dynamic.Depth = depthStencil;
    return std::move(*this);
}

auto BeDrawState::Builder::SetDepthBias(float constant, float slopeScaled) -> Builder&& {
    _dynamic.DepthBias = constant;
    _dynamic.SlopeScaledDepthBias = slopeScaled;
    return std::move(*this);
}

auto BeDrawState::Builder::Build() -> BeDrawState {
    auto state = BeDrawState();
    state._shader = _shader;
    state._staticKeyId = BeBackend::AcquireStaticKeyId(_key);
    state._dynamic = _dynamic;
    return state;
}
