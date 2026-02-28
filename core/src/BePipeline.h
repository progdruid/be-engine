#pragma once

#include <memory>
#include <string>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeTypes.h"

struct BeDrawSlice;
class BeTexture;
class BeMaterial;
class BeShader;
class BeRenderer;
class BeBRPSubmissionBuffer;
struct BePipelineImpl;

class BePipeline {

    expose
    static auto Create(BeRenderer& renderer) -> std::shared_ptr<BePipeline>;

    hide
    std::unique_ptr<BePipelineImpl> _impl;
    BeShaderType _boundShaderType = BeShaderType::None;
    std::shared_ptr<BeShader> _boundShader;

    hide BePipeline();
    expose ~BePipeline();

    expose
    auto BindShader(const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void;
    auto BindMaterialAutomatic(const std::shared_ptr<BeMaterial>& material) -> void;
    auto BindMaterialManual(const std::shared_ptr<BeMaterial>& material, uint8_t materialSlot) -> void;
    auto Clear() -> void;
    auto ClearCache() -> void;

    expose
    auto BindTargets(
        const std::vector<std::weak_ptr<BeTexture>>& renderTargets,
        const BeTexture* depthTarget,
        bool clearRTVs = false
    ) const -> void;
    auto ClearTargets() const -> void;
    auto ResetTarget(const std::shared_ptr<BeTexture>& texture) const -> void;

    expose
    auto Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void;
    auto DrawSlice(const BeDrawSlice& slice) const -> void;
    auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void;

    expose
    auto SetViewport(const BeViewport& viewport) const -> void;
    auto GetViewport() const -> BeViewport;
    auto SetCullMode(BeCullMode mode) const -> void;

    expose
    auto BindMeshBuffers(const BeBRPSubmissionBuffer& submissionBuffer) const -> void;
    auto UnbindMeshBuffers() const -> void;

    expose
    auto SetDepthOnlyTarget(BeTexture* depthTexture) const -> void;
    auto SetCubemapDepthTarget(BeTexture* cubemapTexture, uint32_t face) const -> void;
    auto ClearDepthTarget(BeTexture* depthTexture) const -> void;

    expose
    auto BindBackbuffer(const glm::vec4& clearColor) const -> void;
    auto UnbindBackbuffer() const -> void;

    expose
    auto SetAdditiveBlending() const -> void;
    auto ClearBlendState() const -> void;

    expose
    auto UpdateMaterialBuffers(const std::shared_ptr<BeMaterial>& material) const -> void;

    expose auto GetPlatformImpl() const -> BePipelineImpl* { return _impl.get(); }

    hide auto BindMaterialTextures(const BeMaterial& material) -> void;
};
