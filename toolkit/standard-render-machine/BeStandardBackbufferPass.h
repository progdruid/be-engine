#pragma once
#include <memory>
#include <string>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardBackbufferPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _input;
    std::shared_ptr<BeTexture> _depth;
    std::shared_ptr<BeTexture> _activeInput;
    std::shared_ptr<BeMaterial> _material;
    SenPipeline _pipeline;

    expose
    explicit BeStandardBackbufferPass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> input,
        std::shared_ptr<BeTexture> depth
    );
    ~BeStandardBackbufferPass() override = default;

    expose
    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Backbuffer Pass"; }
};
