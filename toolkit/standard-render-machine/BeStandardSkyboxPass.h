#pragma once
#include <memory>
#include <string>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardSkyboxPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _depth;
    std::shared_ptr<BeTexture> _envCubemap;
    std::shared_ptr<BeTexture> _output;

    std::shared_ptr<BeMaterial> _material;
    SenPipeline _pipeline;

    expose
    explicit BeStandardSkyboxPass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> depth,
        std::shared_ptr<BeTexture> envCubemap,
        std::shared_ptr<BeTexture> output
    );
    ~BeStandardSkyboxPass() override = default;

    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Skybox Pass"; }
};
