#pragma once
#include <memory>
#include <string>
#include <vector>
#include <common.hpp>
#include <SenTypes.h>
#include <Sen.h>

#include "BeRenderPass.h"

class BeMaterial;
class BeTexture;
class BeStandardRenderMachine;

class BeStandardGeometryPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::vector<std::shared_ptr<BeTexture>> _colorTargets;
    std::shared_ptr<BeTexture> _depthTarget;
    std::shared_ptr<BeMaterial> _objectMaterial;

    expose
    explicit BeStandardGeometryPass(
        BeStandardRenderMachine* srm,
        std::vector<std::shared_ptr<BeTexture>> colorTargets,
        std::shared_ptr<BeTexture> depthTarget
    );
    ~BeStandardGeometryPass() override = default;

    expose
    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandList cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Geometry Pass"; }
};
