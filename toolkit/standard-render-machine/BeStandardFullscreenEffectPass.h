#pragma once
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeShader;
class BeStandardRenderMachine;

class BeStandardFullscreenEffectPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    raw_ptr<BeShader> _shader;
    std::shared_ptr<BeMaterial> _material;
    std::vector<std::shared_ptr<BeTexture>> _outputs;
    SenPipeline _pipeline;

    expose
    explicit BeStandardFullscreenEffectPass(
        BeStandardRenderMachine* srm,
        raw_ptr<BeShader> shader,
        std::shared_ptr<BeMaterial> material,
        std::vector<std::shared_ptr<BeTexture>> outputs
    );
    ~BeStandardFullscreenEffectPass() override = default;

    expose
    auto Initialise() -> void override;
    auto Render(SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Fullscreen Effect Pass"; }
};
