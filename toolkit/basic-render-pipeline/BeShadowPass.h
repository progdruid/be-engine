#pragma once
#include <d3d11.h>
#include <memory>
#include <span>
#include <string>
#include <umbrellas/include-glm.h>

#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeRenderPass.h"

class BeBRPSubmissionBuffer;
class BeMaterial;
struct BePointLight;
struct BeDirectionalLight;

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
