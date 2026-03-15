#pragma once
#include <memory>
#include <string>
#include <vector>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeShader;
class BeBRPSubmissionBuffer;

class BeFullscreenEffectPass final : public BeRenderPass {
    expose
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;
    std::vector<std::weak_ptr<BeTexture>> OutputTextures;
    std::weak_ptr<BeShader> Shader;
    std::shared_ptr<BeMaterial> Material;

    hide
    SenPipeline _pipeline;

    expose
    explicit BeFullscreenEffectPass();
    ~BeFullscreenEffectPass() override;

    expose
    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Effect Pass"; }
};
