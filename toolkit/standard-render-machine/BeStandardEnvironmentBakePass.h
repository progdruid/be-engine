#pragma once
#include <array>
#include <memory>
#include <string>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeStandardRenderMachine;

class BeStandardEnvironmentBakePass final : public BeRenderPass {

    hide
    static constexpr uint32_t FaceCount = 6;

    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _equirect;
    std::shared_ptr<BeTexture> _envCubemap;

    SenPipeline _envPipeline;
    std::array<std::shared_ptr<BeMaterial>, FaceCount> _faceMaterials;

    expose
    explicit BeStandardEnvironmentBakePass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> equirect,
        std::shared_ptr<BeTexture> envCubemap
    );
    ~BeStandardEnvironmentBakePass() override = default;

    auto Initialise() -> void override;
    auto Render(SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Environment Bake Pass"; }
};
