#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>
#include <sen-rhi/Sen.h>

#include "BeBackend.h"
#include "BeShaderTools.h"

class BeTexture;
class BeMaterial;
class BeDrawState;

class BePass {
    hide
    struct ReadBinding {
        SenTexture Texture;
        uint32_t BaseMip;
        uint32_t MipCount;
    };

    SenCommandList _list;
    bool _isCompute = false;
    bool _isBegun = false;
    std::vector<ReadBinding> _reads;
    std::vector<SenTexture> _storage;
    std::vector<SenColorAttachment> _colorTargets;
    std::optional<SenDepthAttachment> _depthTarget;
    SenViewport _viewport {};
    SenBuffer _vertexBuffer;
    SenBuffer _indexBuffer;

    BeBackend::FormatSet _formatSet;
    uint32_t _formatSetId = UINT32_MAX;
    const BeDrawState* _state = nullptr;
    uint32_t _staticKeyId = UINT32_MAX;
    BeBackend::StaticKey _overrideKey;
    bool _hasOverrides = false;
    bool _isStateDirty = false;
    SenPipeline _boundPipeline;

    BeBackend::DynamicState _dynamic;

    const BeShaderTools::RootLayout* _rootLayout = nullptr;
    std::array<std::byte, SenMaxRootSize> _rootData {};
    uint32_t _rootWritten = 0;

    expose
    explicit BePass (SenCommandList list);

    auto SetCompute (bool isCompute) -> BePass&;

    auto UseTexture (SenTexture texture, bool useAsStorage = false) -> BePass&;
    auto UseTexture (const std::shared_ptr<BeTexture>& texture, bool useAsStorage = false) -> BePass&;
    auto UseTextureMip (const std::shared_ptr<BeTexture>& texture, uint32_t mipLevel) -> BePass&;
    auto UseTextures (const std::vector<std::shared_ptr<BeTexture>>& textures) -> BePass&;
    auto UseMaterial (const BeMaterial& material) -> BePass&;

    auto AddColorTarget (
        SenView view,
        SenLoadOp loadOp = SenLoadOp::Clear,
        glm::vec4 clearColor = {0, 0, 0, 0}
    ) -> BePass&;
    auto AddColorTarget (
        const std::shared_ptr<BeTexture>& texture,
        SenLoadOp loadOp = SenLoadOp::Clear,
        glm::vec4 clearColor = {0, 0, 0, 0},
        uint8_t mipLevel = 0,
        int16_t arrayLayer = -1
    ) -> BePass&;
    auto AddColorTargets (
        const std::vector<std::shared_ptr<BeTexture>>& textures,
        SenLoadOp loadOp = SenLoadOp::Clear,
        glm::vec4 clearColor = {0, 0, 0, 0}
    ) -> BePass&;

    auto SetDepthTarget (
        SenView view,
        SenLoadOp loadOp = SenLoadOp::Clear,
        float clearDepth = 1.0f,
        uint8_t clearStencil = 0
    ) -> BePass&;
    auto SetDepthTarget (
        const std::shared_ptr<BeTexture>& texture,
        SenLoadOp loadOp = SenLoadOp::Clear,
        float clearDepth = 1.0f,
        int16_t arrayLayer = -1,
        uint8_t clearStencil = 0
    ) -> BePass&;

    auto SetViewport (SenViewport viewport) -> BePass&;

    auto SetVertexBuffer (SenBuffer buffer) -> BePass&;
    auto SetIndexBuffer  (SenBuffer buffer) -> BePass&;

    auto Begin () -> void;
    auto End   () -> void;

    auto SetState (const BeDrawState& state) -> BePass&;
    auto OverrideCull (SenCull mode) -> BePass&;
    auto OverrideFrontFace (SenFrontFace frontFace) -> BePass&;
    auto OverrideDepth (const SenDepthState& depthStencil) -> BePass&;
    auto OverrideDepthBias (float constant, float slopeScaled) -> BePass&;
    auto OverrideFill (SenFill mode) -> BePass&;
    auto OverrideBlend (const SenBlendState& blend) -> BePass&;

    auto Bind (const std::string& link, BeMaterial& material) -> BePass&;

    auto Draw        (uint32_t vertexCount, uint32_t firstVertex = 0) -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
    auto Dispatch    (uint32_t x, uint32_t y, uint32_t z) -> void;
    
    hide
    auto AcquireOverrideKey() -> BeBackend::StaticKey&;
    auto Commit() -> void;
};
