#pragma once
#include <array>
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

class BeStandardEnvironmentBakePass final : public BeRenderPass {

    hide
    static constexpr uint32_t FaceCount = 6;

    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeTexture> _equirect;
    std::shared_ptr<BeTexture> _envCubemap;
    std::shared_ptr<BeTexture> _irradianceCubemap;
    std::shared_ptr<BeTexture> _prefilteredCubemap;
    std::shared_ptr<BeTexture> _brdfLutTexture;

    raw_ptr<BeShader> _envShader;
    BeDrawState _envState;
    std::array<std::shared_ptr<BeMaterial>, FaceCount> _envFaceMaterials;

    raw_ptr<BeShader> _irradianceShader;
    BeDrawState _irradianceState;
    std::array<std::shared_ptr<BeMaterial>, FaceCount> _irradianceFaceMaterials;

    raw_ptr<BeShader> _prefilterShader;
    BeDrawState _prefilterState;
    std::vector<std::array<std::shared_ptr<BeMaterial>, FaceCount>> _prefilterFaceMaterials;

    raw_ptr<BeShader> _brdfLutShader;
    BeDrawState _brdfLutState;
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

    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Environment Bake Pass"; }
};
