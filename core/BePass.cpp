#include "BePass.h"

#include <utility>

#include "BeMaterial.h"
#include "BeTexture.h"
#include <sen-rhi/SenBackend.h>
#include <umbrellas/include-libassert.h>

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

auto BePass::AddColorTarget(SenTexture texture, SenLoadOp loadOp, glm::vec4 clearColor, uint8_t mipLevel, int8_t cubemapFace) -> BePass& {
    be_assert(texture.IsValid(), "BePass::AddColorTarget: invalid texture handle");
    _colorTargets.push_back(SenColorAttachment{
        .Texture     = texture,
        .MipLevel    = mipLevel,
        .CubemapFace = cubemapFace,
        .LoadOp      = loadOp,
        .ClearColor  = clearColor,
    });
    return *this;
}

auto BePass::AddColorTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, glm::vec4 clearColor, uint8_t mipLevel, int8_t cubemapFace) -> BePass& {
    be_assert(texture != nullptr, "BePass::AddColorTarget: null texture");
    return AddColorTarget(texture->Handle, loadOp, clearColor, mipLevel, cubemapFace);
}

auto BePass::AddColorTargets(const std::vector<std::shared_ptr<BeTexture>>& textures, SenLoadOp loadOp, glm::vec4 clearColor) -> BePass& {
    for (const auto& texture : textures) {
        AddColorTarget(texture, loadOp, clearColor);
    }
    return *this;
}

auto BePass::SetDepthTarget(SenTexture texture, SenLoadOp loadOp, float clearDepth, int8_t cubemapFace, uint8_t clearStencil) -> BePass& {
    be_assert(texture.IsValid(), "BePass::SetDepthTarget: invalid texture handle");
    _depthTarget = SenDepthAttachment{
        .Texture      = texture,
        .CubemapFace  = cubemapFace,
        .LoadOp       = loadOp,
        .ClearDepth   = clearDepth,
        .ClearStencil = clearStencil,
    };
    return *this;
}

auto BePass::SetDepthTarget(const std::shared_ptr<BeTexture>& texture, SenLoadOp loadOp, float clearDepth, int8_t cubemapFace, uint8_t clearStencil) -> BePass& {
    be_assert(texture != nullptr, "BePass::SetDepthTarget: null texture");
    return SetDepthTarget(texture->Handle, loadOp, clearDepth, cubemapFace, clearStencil);
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

    auto& cmd = SenBackend::GetCommandBuffer();

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
    cmd.TransitionTextures(transitions);

    if (!_isCompute) {
        cmd.BeginPass({
            .ColorAttachments = _colorTargets,
            .DepthAttachment  = _depthTarget,
            .Viewport         = _viewport,
        });
    }
}

auto BePass::End() -> void {
    if (!_isCompute) {
        SenBackend::GetCommandBuffer().EndPass();
    }
}
