#pragma once
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeDrawState.h"
#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
struct BeShader;
class BeStandardRenderMachine;

class BeStandardBloomPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _inputHDR;
    std::shared_ptr<BeTexture> _bloomTexture;
    std::shared_ptr<BeTexture> _output;
    std::shared_ptr<BeTexture> _dirtTexture;
    uint32_t _mipCount;

    std::shared_ptr<BeMaterial> _brightMaterial;
    raw_ptr<BeShader> _brightShader;
    BeDrawState _brightState;
    std::vector<std::shared_ptr<BeMaterial>> _downsampleMaterials;
    std::vector<std::shared_ptr<BeMaterial>> _upsampleMaterials;
    raw_ptr<BeShader> _downsampleShader;
    raw_ptr<BeShader> _upsampleShader;
    BeDrawState _downsampleState;
    BeDrawState _upsampleState;
    std::shared_ptr<BeMaterial> _addMaterial;
    raw_ptr<BeShader> _addShader;
    BeDrawState _addState;

    expose
    explicit BeStandardBloomPass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> inputHDR,
        std::shared_ptr<BeTexture> bloomTexture,
        std::shared_ptr<BeTexture> output,
        std::shared_ptr<BeTexture> dirtTexture,
        uint32_t mipCount
    );
    ~BeStandardBloomPass() override = default;

    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandList cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Bloom Pass"; }

    hide
    auto RenderBrightPass(SenCommandList cmd) const -> void;
    auto RenderDownsamplePasses(SenCommandList cmd) const -> void;
    auto RenderUpsamplePasses(SenCommandList cmd) const -> void;
    auto RenderAddPass(BeRenderer& renderer, SenCommandList cmd) const -> void;
};
