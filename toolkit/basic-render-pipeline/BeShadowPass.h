#pragma once

#include <memory>
#include <string>
#include <umbrellas/include-glm.h>

#include "BeRenderPass.h"

class BeBRPSubmissionBuffer;
class BeMaterial;
struct BeBRPSunLightEntry;
struct BeBRPPointLightEntry;

class BeShadowPass final : public BeRenderPass {

    expose
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;

    hide
    std::shared_ptr<BeMaterial> _objectMaterial;

    expose
    explicit BeShadowPass() = default;
    ~BeShadowPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Shadow Pass"; }

    hide
    auto RenderDirectionalShadows(const BeBRPSunLightEntry& sunLight, const BeBRPSubmissionBuffer& submissionBuffer) const -> void;
    auto RenderPointLightShadows(const BeBRPPointLightEntry& pointLight, const BeBRPSubmissionBuffer& submissionBuffer) const -> void;
    auto CalculatePointLightFaceViewProjection(const BeBRPPointLightEntry& pointLight, int faceIndex) const -> glm::mat4;
};
