#pragma once
#include <memory>
#include <string>
#include <umbrellas/access-modifiers.hpp>
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
    std::shared_ptr<BeTexture> _activeInput;
    glm::vec3 _clearColor;
    std::shared_ptr<BeMaterial> _material;
    SenPipeline _pipeline;

    expose
    explicit BeStandardBackbufferPass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> input,
        glm::vec3 clearColor
    );
    ~BeStandardBackbufferPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Backbuffer Pass"; }
};
