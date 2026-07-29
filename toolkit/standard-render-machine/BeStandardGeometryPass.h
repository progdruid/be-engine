#pragma once
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeStandardRenderMachine;

class BeStandardGeometryPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::vector<std::shared_ptr<BeTexture>> _colorTargets;
    std::shared_ptr<BeTexture> _depthTarget;

    expose
    explicit BeStandardGeometryPass(
        BeStandardRenderMachine* srm,
        std::vector<std::shared_ptr<BeTexture>> colorTargets,
        std::shared_ptr<BeTexture> depthTarget
    );
    ~BeStandardGeometryPass() override = default;

    expose
    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Geometry Pass"; }
};
