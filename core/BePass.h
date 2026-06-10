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
    std::vector<SenTexture> _reads;
    std::vector<SenColorAttachment> _colorTargets;
    std::optional<SenDepthAttachment> _depthTarget;
    SenViewport _viewport {};

    expose
    auto AddReadTexture (SenTexture texture) -> BePass&;
    auto AddReadTexture (const std::shared_ptr<BeTexture>& texture) -> BePass&;
    auto AddReadTextures (const std::vector<std::shared_ptr<BeTexture>>& textures) -> BePass&;
    auto AddReadMaterial (const BeMaterial& material) -> BePass&;

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
