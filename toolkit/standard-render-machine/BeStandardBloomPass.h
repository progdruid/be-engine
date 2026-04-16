#pragma once
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardBloomPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _inputHDR;
    std::vector<std::shared_ptr<BeTexture>> _mipTextures;
    std::shared_ptr<BeTexture> _output;
    std::shared_ptr<BeTexture> _dirtTexture;
    uint32_t _mipCount;

    std::shared_ptr<BeMaterial> _brightMaterial;
    SenPipeline _brightPipeline;
    std::vector<std::shared_ptr<BeMaterial>> _downsampleMaterials;
    std::vector<std::shared_ptr<BeMaterial>> _upsampleMaterials;
    SenPipeline _downsamplePipeline;
    SenPipeline _upsamplePipeline;
    std::shared_ptr<BeMaterial> _addMaterial;
    SenPipeline _addPipeline;

    expose
    explicit BeStandardBloomPass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> inputHDR,
        std::vector<std::shared_ptr<BeTexture>> mipTextures,
        std::shared_ptr<BeTexture> output,
        std::shared_ptr<BeTexture> dirtTexture,
        uint32_t mipCount
    );
    ~BeStandardBloomPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Bloom Pass"; }

    hide
    auto RenderBrightPass() -> void;
    auto RenderDownsamplePasses() -> void;
    auto RenderUpsamplePasses() -> void;
    auto RenderAddPass() -> void;
};
