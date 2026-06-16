#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

class BeTexture;
class BeMaterial;

class BePass {
    hide
    struct ReadBinding {
        SenTexture Texture;
        uint32_t BaseMip;
        uint32_t MipCount;
    };

    bool _isCompute = false;
    std::vector<ReadBinding> _reads;
    std::vector<SenTexture> _storageTextures;
    std::vector<SenColorAttachment> _colorTargets;
    std::optional<SenDepthAttachment> _depthTarget;
    SenViewport _viewport {};

    expose
    auto SetCompute (bool isCompute) -> BePass&;

    auto UseTexture (SenTexture texture, bool useAsStorage = false) -> BePass&;
    auto UseTexture (const std::shared_ptr<BeTexture>& texture, bool useAsStorage = false) -> BePass&;
    auto UseTextureMip (const std::shared_ptr<BeTexture>& texture, uint32_t mipLevel) -> BePass&;
    auto UseTextures (const std::vector<std::shared_ptr<BeTexture>>& textures) -> BePass&;
    auto UseMaterial (const BeMaterial& material) -> BePass&;

    auto AddColorTarget (
        SenTexture texture, 
        SenLoadOp loadOp = SenLoadOp::Clear, 
        glm::vec4 clearColor = {0, 0, 0, 0}, 
        uint8_t mipLevel = 0,
        int8_t cubemapFace = -1
    ) -> BePass&;
    auto AddColorTarget (
        const std::shared_ptr<BeTexture>& texture,
        SenLoadOp loadOp = SenLoadOp::Clear,
        glm::vec4 clearColor = {0, 0, 0, 0},
        uint8_t mipLevel = 0,
        int8_t cubemapFace = -1
    ) -> BePass&;
    auto AddColorTargets (
        const std::vector<std::shared_ptr<BeTexture>>& textures,
        SenLoadOp loadOp = SenLoadOp::Clear,
        glm::vec4 clearColor = {0, 0, 0, 0}
    ) -> BePass&;

    auto SetDepthTarget (
        SenTexture texture, 
        SenLoadOp loadOp = SenLoadOp::Clear, 
        float clearDepth = 1.0f, 
        int8_t cubemapFace = -1, 
        uint8_t clearStencil = 0
    ) -> BePass&;
    auto SetDepthTarget (
        const std::shared_ptr<BeTexture>& texture, 
        SenLoadOp loadOp = SenLoadOp::Clear, 
        float clearDepth = 1.0f, 
        int8_t cubemapFace = -1, 
        uint8_t clearStencil = 0
    ) -> BePass&;

    auto SetViewport    (SenViewport viewport) -> BePass&;

    auto Begin () -> void;
    auto End   () -> void;
};
