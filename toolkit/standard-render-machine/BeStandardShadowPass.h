#pragma once
#include <string>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeShader;
class BeStandardRenderMachine;
struct BeSRMSunLightEntry;
struct BeSRMPointLightEntry;

class BeStandardShadowPass final : public BeRenderPass {

    hide
    BeStandardRenderMachine* _srm;

    expose
    explicit BeStandardShadowPass(BeStandardRenderMachine* srm);
    ~BeStandardShadowPass() override = default;

    auto Initialise() -> void override {}
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Standard Shadow Pass"; }

    hide
    auto RenderDirectionalShadows(const BeSRMSunLightEntry& sunLight) -> void;
    auto RenderPointLightShadows(const BeSRMPointLightEntry& pointLight) -> void;
    auto CalculatePointLightFaceViewProjection(const BeSRMPointLightEntry& pointLight, int faceIndex) const -> glm::mat4;
};
