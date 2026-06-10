#include "BePass.h"

#include <utility>

#include "BeMaterial.h"
#include "BeTexture.h"
#include <sen-rhi/SenBackend.h>
#include <umbrellas/include-libassert.h>

auto BePass::AddReadTexture(SenTexture texture) -> BePass& {
    be_assert(texture.IsValid(), "BePass::AddReadTexture: invalid texture handle");
    _reads.push_back(texture);
    return *this;
}

auto BePass::AddReadTexture(const std::shared_ptr<BeTexture>& texture) -> BePass& {
    be_assert(texture != nullptr, "BePass::AddReadTexture: null texture");
    return AddReadTexture(texture->Handle);
}

auto BePass::AddReadTextures(const std::vector<std::shared_ptr<BeTexture>>& textures) -> BePass& {
    for (const auto& texture : textures) {
        AddReadTexture(texture);
    }
    return *this;
}

auto BePass::AddReadMaterial(const BeMaterial& material) -> BePass& {
    for (const auto& [texture, slot] : material.GetTextures()) {
        AddReadTexture(texture);
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
    be_assert(
        !_colorTargets.empty() || _depthTarget.has_value(),
        "BePass::Begin: pass has no render targets (need at least one color target or a depth target)"
    );

    auto& cmd = SenBackend::GetCommandBuffer();

    std::vector<std::pair<SenTexture, SenResourceState>> transitions;
    transitions.reserve(_reads.size() + _colorTargets.size() + 1);
    for (const auto texture : _reads) {
        transitions.emplace_back(texture, SenResourceState::ShaderRead);
    }
    for (const auto& target : _colorTargets) {
        transitions.emplace_back(target.Texture, SenResourceState::ColorAttachment);
    }
    if (_depthTarget) {
        transitions.emplace_back(_depthTarget->Texture, SenResourceState::DepthAttachment);
    }
    cmd.TransitionTextures(transitions);

    cmd.BeginPass({
        .ColorAttachments = _colorTargets,
        .DepthAttachment  = _depthTarget,
        .Viewport         = _viewport,
    });
}

auto BePass::End() -> void {
    SenBackend::GetCommandBuffer().EndPass();
}
