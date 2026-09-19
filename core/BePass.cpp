#include "BePass.h"

#include <utility>

#include "BeDrawState.h"
#include "BeMaterial.h"
#include "BeRoot.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <sen-rhi/SenBackend.h>
#include <umbrellas/include-libassert.h>

BePass::BePass(SenCommandBuffer& cmd)
    : _cmd(cmd)
{}

auto BePass::SetCompute(bool isCompute) -> BePass& {
    _isCompute = isCompute;
    return *this;
}

auto BePass::UseTexture(SenTexture texture, bool useAsStorage) -> BePass& {
    be_assert(texture.IsValid(), "BePass::AddReadTexture: invalid texture handle");
    if (useAsStorage) {
        _storageTextures.push_back(texture);
    } else {
        _reads.push_back({ texture, 0, SenCommandBuffer::TextureTransition::AllMips });
    }
    return *this;
}

auto BePass::UseTexture(const std::shared_ptr<BeTexture>& texture, bool useAsStorage) -> BePass& {
    be_assert(texture != nullptr, "BePass::AddReadTexture: null texture");
    return UseTexture(texture->Handle, useAsStorage);
}

auto BePass::UseTextureMip(const std::shared_ptr<BeTexture>& texture, uint32_t mipLevel) -> BePass& {
    be_assert(texture != nullptr, "BePass::UseTextureMip: null texture");
    _reads.push_back({ texture->Handle, mipLevel, 1 });
    return *this;
}

auto BePass::UseTextures(const std::vector<std::shared_ptr<BeTexture>>& textures) -> BePass& {
    for (const auto& texture : textures) {
        UseTexture(texture);
    }
    return *this;
}

auto BePass::UseMaterial(const BeMaterial& material) -> BePass& {
    for (const auto& binding : material.GetTextures()) {
        UseTexture(binding.Texture, binding.IsStorage);
    }
    return *this;
}

auto BePass::AddColorTarget(SenTexture texture, SenFormat format, SenLoadOp loadOp, glm::vec4 clearColor, uint8_t mipLevel, int16_t arrayLayer) -> BePass& {
    be_assert(texture.IsValid(), "BePass::AddColorTarget: invalid texture handle");
    be_assert(_colorTargets.size() < _formatSet.ColorFormats.size(), "BePass::AddColorTarget: too many color targets");
    _formatSet.ColorFormats[_colorTargets.size()] = format;
    _colorTargets.push_back(SenColorAttachment{
        .Texture     = texture,
        .MipLevel    = mipLevel,
        .Layer       = arrayLayer,
        .LoadOp      = loadOp,
        .ClearColor  = clearColor,
    });
    return *this;
}

auto BePass::AddColorTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, glm::vec4 clearColor, uint8_t mipLevel, int16_t arrayLayer) -> BePass& {
    be_assert(texture != nullptr, "BePass::AddColorTarget: null texture");
    return AddColorTarget(texture->Handle, texture->Format, loadOp, clearColor, mipLevel, arrayLayer);
}

auto BePass::AddColorTargets(const std::vector<std::shared_ptr<BeTexture>>& textures, SenLoadOp loadOp, glm::vec4 clearColor) -> BePass& {
    for (const auto& texture : textures) {
        AddColorTarget(texture, loadOp, clearColor);
    }
    return *this;
}

auto BePass::SetDepthTarget(SenTexture texture, SenFormat format, SenLoadOp loadOp, float clearDepth, int16_t arrayLayer, uint8_t clearStencil) -> BePass& {
    be_assert(texture.IsValid(), "BePass::SetDepthTarget: invalid texture handle");
    _formatSet.DepthFormat = format;
    _depthTarget = SenDepthAttachment{
        .Texture      = texture,
        .Layer        = arrayLayer,
        .LoadOp       = loadOp,
        .ClearDepth   = clearDepth,
        .ClearStencil = clearStencil,
    };
    return *this;
}

auto BePass::SetDepthTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, float clearDepth, int16_t arrayLayer, uint8_t clearStencil) -> BePass& {
    be_assert(texture != nullptr, "BePass::SetDepthTarget: null texture");
    return SetDepthTarget(texture->Handle, texture->Format, loadOp, clearDepth, arrayLayer, clearStencil);
}

auto BePass::SetViewport(SenViewport viewport) -> BePass& {
    _viewport = viewport;
    return *this;
}

auto BePass::Begin() -> void {
    const bool hasTargets = !_colorTargets.empty() || _depthTarget.has_value();
    be_assert(
        _isCompute || hasTargets,
        "BePass::Begin: graphics pass has no render targets (need at least one color target or a depth target)"
    );
    be_assert(
        !_isCompute || !hasTargets,
        "BePass::Begin: compute pass cannot have render targets"
    );

    using Transition = SenCommandBuffer::TextureTransition;
    std::vector<Transition> transitions;
    transitions.reserve(_reads.size() + _storageTextures.size() + _colorTargets.size() + 1);
    for (const auto& read : _reads) {
        transitions.push_back({ read.Texture, SenResourceState::ShaderRead, read.BaseMip, read.MipCount });
    }
    for (const auto texture : _storageTextures) {
        transitions.push_back({ texture, SenResourceState::UnorderedAccess });
    }
    for (const auto& target : _colorTargets) {
        transitions.push_back({ target.Texture, SenResourceState::ColorAttachment, target.MipLevel, 1 });
    }
    if (_depthTarget) {
        transitions.push_back({ _depthTarget->Texture, SenResourceState::DepthAttachment });
    }
    _cmd.TransitionTextures(transitions);
    _formatSetId = BeBackend::AcquireFormatSetId(_formatSet);

    if (!_isCompute) {
        _cmd.BeginPass({
            .ColorAttachments = _colorTargets,
            .DepthAttachment  = _depthTarget,
            .Viewport         = _viewport,
        });
    }
}

auto BePass::End() -> void {
    if (!_isCompute) {
        _cmd.EndPass();
    }
}

auto BePass::SetState(const BeDrawState& state) -> BePass& {
    be_assert(state.IsValid(), "BePass::SetState: state was never built");
    _state = &state;
    _staticKeyId = state.GetStaticKeyId();
    _hasOverrides = false;
    _isStateDirty = true;
    return *this;
}

auto BePass::AcquireOverrideKey() -> BeBackend::StaticKey& {
    be_assert(_state != nullptr, "BePass: override without a state");
    if (!_hasOverrides) {
        _overrideKey = BeBackend::GetStaticKey(_staticKeyId);
        _hasOverrides = true;
    }
    _isStateDirty = true;
    return _overrideKey;
}

auto BePass::OverrideCull(SenCullMode mode) -> BePass& {
    AcquireOverrideKey().RasterizerState.CullMode = mode;
    return *this;
}

auto BePass::OverrideFill(SenFillMode mode) -> BePass& {
    AcquireOverrideKey().RasterizerState.FillMode = mode;
    return *this;
}

auto BePass::OverrideBlend(const SenBlendState& blend) -> BePass& {
    AcquireOverrideKey().BlendState = blend;
    return *this;
}

auto BePass::OverrideDepthStencil(const SenDepthStencilState& depthStencil) -> BePass& {
    AcquireOverrideKey().DepthStencilState = depthStencil;
    return *this;
}

auto BePass::Push(const BeRoot& root) -> void {
    be_assert(_formatSetId != UINT32_MAX, "BePass::Push: pass has not begun");
    be_assert(_state != nullptr, "BePass::Push: no state set");
    be_assert(root._layout == &_state->GetShader().RootLayout, "BePass::Push: root was built for another shader");

    if (_isStateDirty) {
        if (_hasOverrides) {
            _staticKeyId = BeBackend::AcquireStaticKeyId(_overrideKey);
        }
        const auto pipeline = BeBackend::GetPipeline(_state->GetShader(), _staticKeyId, _formatSetId);
        if (pipeline.ID != _boundPipeline.ID) {
            _cmd.SetPipeline(pipeline);
            _boundPipeline = pipeline;
        }
        _isStateDirty = false;
    }
    root.Push(_cmd);
}
