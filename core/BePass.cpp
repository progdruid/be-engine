#include "BePass.h"

#include <cstring>
#include <utility>

#include "BeDrawState.h"
#include "BeMaterial.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <sen-rhi/SenBackend.h>
#include <umbrellas/include-libassert.h>

BePass::BePass(SenCommandList list)
    : _list(list)
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
        _reads.push_back({ texture, 0, SenAllMips });
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

auto BePass::AddColorTarget(SenView view, SenLoadOp loadOp, glm::vec4 clearColor) -> BePass& {
    be_assert(view.IsValid(), "BePass::AddColorTarget: invalid view handle");
    be_assert(_colorTargets.size() < _formatSet.ColorFormats.size(), "BePass::AddColorTarget: too many color targets");
    _formatSet.ColorFormats[_colorTargets.size()] = SenBackend::GetViewFormat(view);
    _colorTargets.push_back(SenColorAttachment{
        .View       = view,
        .LoadOp     = loadOp,
        .ClearColor = clearColor,
    });
    return *this;
}

auto BePass::AddColorTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, glm::vec4 clearColor, uint8_t mipLevel, int16_t arrayLayer) -> BePass& {
    be_assert(texture != nullptr, "BePass::AddColorTarget: null texture");
    return AddColorTarget(texture->GetColorTargetView(mipLevel, arrayLayer), loadOp, clearColor);
}

auto BePass::AddColorTargets(const std::vector<std::shared_ptr<BeTexture>>& textures, SenLoadOp loadOp, glm::vec4 clearColor) -> BePass& {
    for (const auto& texture : textures) {
        AddColorTarget(texture, loadOp, clearColor);
    }
    return *this;
}

auto BePass::SetDepthTarget(SenView view, SenLoadOp loadOp, float clearDepth, uint8_t clearStencil) -> BePass& {
    be_assert(view.IsValid(), "BePass::SetDepthTarget: invalid view handle");
    _formatSet.DepthFormat = SenBackend::GetViewFormat(view);
    _depthTarget = SenDepthAttachment{
        .View         = view,
        .LoadOp       = loadOp,
        .ClearDepth   = clearDepth,
        .ClearStencil = clearStencil,
    };
    return *this;
}

auto BePass::SetDepthTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, float clearDepth, int16_t arrayLayer, uint8_t clearStencil) -> BePass& {
    be_assert(texture != nullptr, "BePass::SetDepthTarget: null texture");
    return SetDepthTarget(texture->GetDepthTargetView(arrayLayer), loadOp, clearDepth, clearStencil);
}

auto BePass::SetViewport(SenViewport viewport) -> BePass& {
    _viewport = viewport;
    return *this;
}

auto BePass::SetVertexBuffer(SenBuffer buffer) -> BePass& {
    _vertexBuffer = buffer;
    return *this;
}

auto BePass::SetIndexBuffer(SenBuffer buffer) -> BePass& {
    _indexBuffer = buffer;
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

    std::vector<SenTextureTransition> transitions;
    transitions.reserve(_reads.size() + _storageTextures.size() + _colorTargets.size() + 1);
    for (const auto& read : _reads) {
        transitions.push_back({ read.Texture, SenResourceState::ShaderRead, read.BaseMip, read.MipCount });
    }
    for (const auto texture : _storageTextures) {
        transitions.push_back({ texture, SenResourceState::UnorderedAccess });
    }
    for (const auto& target : _colorTargets) {
        const auto& view = SenBackend::GetViewDesc(target.View);
        transitions.push_back({ view.Texture, SenResourceState::ColorAttachment, view.BaseMip, view.MipCount });
    }
    if (_depthTarget) {
        const auto& view = SenBackend::GetViewDesc(_depthTarget->View);
        transitions.push_back({ view.Texture, SenResourceState::DepthAttachment, view.BaseMip, view.MipCount });
    }
    SenCmd::TransitionTextures(_list, transitions);
    _formatSetId = BeBackend::AcquireFormatSetId(_formatSet);

    if (!_isCompute) {
        SenCmd::BeginPass(_list, {
            .ColorAttachments = _colorTargets,
            .DepthAttachment  = _depthTarget,
            .Viewport         = _viewport,
        });
    }

    if (_vertexBuffer.IsValid()) { SenCmd::SetVertexBuffer(_list, _vertexBuffer); }
    if (_indexBuffer.IsValid())  { SenCmd::SetIndexBuffer(_list, _indexBuffer); }

    _isBegun = true;
}

auto BePass::End() -> void {
    if (!_isCompute) {
        SenCmd::EndPass(_list);
    }
    _isBegun = false;
}

auto BePass::SetState(const BeDrawState& state) -> BePass& {
    be_assert(state.IsValid(), "BePass::SetState: state was never built");
    _state = &state;
    _staticKeyId = state.GetStaticKeyId();
    _hasOverrides = false;
    _isStateDirty = true;

    _rootLayout = &state.GetShader().RootLayout;
    be_assert(
        _rootLayout->Size <= SenMaxRootConstantSize,
        "BePass::SetState: root exceeds push-constant capacity",
        _rootLayout->Size
    );
    be_assert(_rootLayout->Fields.size() <= 32, "BePass::SetState: too many root fields to track");
    _rootWritten = 0;
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

auto BePass::Bind(const std::string& link, BeMaterial& material) -> BePass& {
    be_assert(_rootLayout != nullptr, "BePass::Bind: no state set");

    for (size_t i = 0; i < _rootLayout->Fields.size(); ++i) {
        const auto& field = _rootLayout->Fields[i];
        if (field.Link != link) {
            continue;
        }

        switch (field.Kind) {
            case BeShaderTools::RootFieldKind::Pointer: {
                const uint64_t address = material.GetCbufferAddress().Value;
                std::memcpy(_rootData.data() + field.Offset, &address, sizeof(address));
                break;
            }
            case BeShaderTools::RootFieldKind::TextureIndex: {
                const uint32_t index = material.GetTextureSlot(field.PropertyName);
                std::memcpy(_rootData.data() + field.Offset, &index, sizeof(index));
                break;
            }
            case BeShaderTools::RootFieldKind::SamplerIndex: {
                const uint32_t index = material.GetSamplerSlot(field.PropertyName);
                std::memcpy(_rootData.data() + field.Offset, &index, sizeof(index));
                break;
            }
        }
        _rootWritten |= (static_cast<uint32_t>(1) << i);
    }
    return *this;
}

auto BePass::Commit() -> void {
    be_assert(_isBegun, "BePass: draw before Begin");
    be_assert(_state != nullptr, "BePass: draw without a state");

    if (_isStateDirty) {
        if (_hasOverrides) {
            _staticKeyId = BeBackend::AcquireStaticKeyId(_overrideKey);
        }
        const auto pipeline = BeBackend::GetPipeline(_state->GetShader(), _staticKeyId, _formatSetId);
        if (pipeline != _boundPipeline) {
            SenCmd::SetPipeline(_list, pipeline);
            _boundPipeline = pipeline;
        }
        _isStateDirty = false;
    }

    if (_rootLayout->Size == 0) {
        return;
    }
    for (size_t i = 0; i < _rootLayout->Fields.size(); ++i) {
        be_assert(
            _rootWritten & (static_cast<uint32_t>(1) << i),
            "BePass: root field not bound before draw",
            _rootLayout->Fields[i].FieldName
        );
    }
    SenCmd::PushRoot(_list, _rootData.data(), _rootLayout->Size);
    _rootWritten = 0;
}

auto BePass::Draw(uint32_t vertexCount, uint32_t firstVertex) -> void {
    Commit();
    SenCmd::Draw(_list, vertexCount, firstVertex);
}

auto BePass::DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void {
    Commit();
    SenCmd::DrawIndexed(_list, indexCount, firstIndex, baseVertex);
}

auto BePass::Dispatch(uint32_t x, uint32_t y, uint32_t z) -> void {
    be_assert(_isCompute, "BePass::Dispatch: pass is not a compute pass");
    Commit();
    SenCmd::Dispatch(_list, x, y, z);
}
