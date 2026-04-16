#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardLightingPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::vector<std::shared_ptr<BeTexture>> _gbufferInputs;
    std::shared_ptr<BeTexture> _depthInput;
    std::shared_ptr<BeTexture> _output;

    std::shared_ptr<BeMaterial> _directionalLightMaterial;
    SenPipeline _directionalLightPipeline;
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _pointLightMaterials;
    SenPipeline _pointLightPipeline;
    std::shared_ptr<BeMaterial> _emissiveMaterial;
    SenPipeline _emissivePipeline;

    expose
    explicit BeStandardLightingPass(
        BeStandardRenderMachine* srm,
        std::vector<std::shared_ptr<BeTexture>> gbufferInputs,
        std::shared_ptr<BeTexture> depthInput,
        std::shared_ptr<BeTexture> output
    );
    ~BeStandardLightingPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Lighting Pass"; }
};
