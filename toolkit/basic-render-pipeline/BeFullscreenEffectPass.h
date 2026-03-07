#pragma once
#include <memory>
#include <string>
#include <vector>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"
#include "BeMaterialBinding.h"

class BeTexture;
class BeMaterial;
class BeShader;

class BeFullscreenEffectPass final : public BeRenderPass {
    expose
    std::vector<std::weak_ptr<BeTexture>> OutputTextures;
    std::weak_ptr<BeShader> Shader;
    std::shared_ptr<BeMaterial> Material;

    hide
    SenPipeline _pipeline;
    BeMaterialBinding _binding;

    expose
    explicit BeFullscreenEffectPass();
    ~BeFullscreenEffectPass() override;

    expose
    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Effect Pass"; }
};
