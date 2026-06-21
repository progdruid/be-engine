#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
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
    std::shared_ptr<BeTexture> _irradianceCubemap;
    std::shared_ptr<BeTexture> _prefilteredCubemap;
    std::shared_ptr<BeTexture> _brdfLutTexture;

    SenPipeline _envPipeline;
    std::array<std::shared_ptr<BeMaterial>, FaceCount> _envFaceMaterials;

    SenPipeline _irradiancePipeline;
    std::array<std::shared_ptr<BeMaterial>, FaceCount> _irradianceFaceMaterials;

    SenPipeline _prefilterPipeline;
    std::vector<std::array<std::shared_ptr<BeMaterial>, FaceCount>> _prefilterFaceMaterials;

    SenPipeline _brdfLutPipeline;
    std::shared_ptr<BeMaterial> _brdfLutMaterial;

    expose
    explicit BeStandardEnvironmentBakePass(
        BeStandardRenderMachine* srm,
        std::shared_ptr<BeTexture> equirect,
        std::shared_ptr<BeTexture> envCubemap,
        std::shared_ptr<BeTexture> irradianceCubemap,
        std::shared_ptr<BeTexture> prefilteredCubemap,
        std::shared_ptr<BeTexture> brdfLutTexture
    );
    ~BeStandardEnvironmentBakePass() override = default;

    auto Initialise() -> void override;
    auto Render(SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Environment Bake Pass"; }
};
