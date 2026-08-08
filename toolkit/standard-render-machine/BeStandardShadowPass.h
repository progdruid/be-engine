#pragma once
#include <memory>
#include <string>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeMaterial;
class BeShader;
class BeTexture;
class BeStandardRenderMachine;
struct BeSRMSunLightEntry;
struct BeSRMPointLightEntry;

class BeStandardShadowPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;
    std::shared_ptr<BeMaterial> _objectMaterial;

    expose
    explicit BeStandardShadowPass(BeStandardRenderMachine* srm);
    ~BeStandardShadowPass() override = default;

    expose
    auto Initialise(BeRenderer& renderer) -> void override;
    auto Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Shadow Pass"; }

    hide
    auto RenderDirectionalShadows(
        SenCommandBuffer& cmd, 
        const BeSRMSunLightEntry& sunLight, 
        const std::shared_ptr<BeTexture>& shadowArray, 
        uint32_t slice
    ) const -> void;
    auto RenderPointLightShadows(
        SenCommandBuffer& cmd, 
        const BeSRMPointLightEntry& pointLight, 
        const std::shared_ptr<BeTexture>& shadowArray, 
        uint32_t slice
    ) const -> void;
    auto CalculatePointLightFaceViewProjection(const BeSRMPointLightEntry& pointLight, int faceIndex) const -> glm::mat4;
};
