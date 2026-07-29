#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"
#include "BeMaterialScheme.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardLightingPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::vector<std::shared_ptr<BeTexture>> _gbufferInputs;
    std::shared_ptr<BeTexture> _depthInput;
    std::shared_ptr<BeTexture> _irradianceCubemap;
    std::shared_ptr<BeTexture> _prefilteredCubemap;
    std::shared_ptr<BeTexture> _brdfLutTexture;
    std::shared_ptr<BeTexture> _output;

    BeMaterialScheme _directionalLightScheme;
    std::vector<std::shared_ptr<BeMaterial>> _directionalLightMaterials;
    SenPipeline _directionalLightPipeline;
    BeMaterialScheme _pointLightScheme;
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _pointLightMaterials;
    SenPipeline _pointLightPipeline;
    std::shared_ptr<BeMaterial> _emissiveMaterial;
    SenPipeline _emissivePipeline;
    std::shared_ptr<BeMaterial> _ambientMaterial;
    SenPipeline _ambientPipeline;

    expose
    explicit BeStandardLightingPass(
        BeStandardRenderMachine* srm,
        std::vector<std::shared_ptr<BeTexture>> gbufferInputs,
        std::shared_ptr<BeTexture> depthInput,
        std::shared_ptr<BeTexture> irradianceCubemap,
        std::shared_ptr<BeTexture> prefilteredCubemap,
        std::shared_ptr<BeTexture> brdfLutTexture,
        std::shared_ptr<BeTexture> output
    );
    ~BeStandardLightingPass() override = default;

    auto Initialise() -> void override;
    auto Render(SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Lighting Pass"; }
};
