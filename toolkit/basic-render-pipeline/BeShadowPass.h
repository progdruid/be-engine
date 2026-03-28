#pragma once
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <functional>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

#include "BeBRPSubmissionBuffer.h"
#include "BeRenderPass.h"

class BeBRPSubmissionBuffer;
class BeMaterial;
struct BePointLight;
struct BeDirectionalLight;

class BeShadowPass final : public BeRenderPass {

    hide
    struct PipelineKey {
        BeShader* Shader;
        bool TwoSided;

        auto operator==(const PipelineKey& other) const -> bool {
            return Shader == other.Shader && TwoSided == other.TwoSided;
        }
    };

    struct PipelineKeyHash {
        auto operator()(const PipelineKey& key) const -> size_t {
            return std::hash<BeShader*>()(key.Shader) ^ (std::hash<bool>()(key.TwoSided) << 1);
        }
    };

    expose
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;

    hide
    std::unordered_map<PipelineKey, SenPipeline, PipelineKeyHash> _shaderPipelines;

    expose
    explicit BeShadowPass() = default;
    ~BeShadowPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Shadow Pass"; }

    hide
    auto RenderDirectionalShadows(const BeBRPSunLightEntry& sunLight, BeBRPSubmissionBuffer& submissionBuffer) -> void;
    auto RenderPointLightShadows(const BeBRPPointLightEntry& pointLight, BeBRPSubmissionBuffer& submissionBuffer) -> void;

    auto CalculatePointLightFaceViewProjection(const BeBRPPointLightEntry& pointLight, int faceIndex) const -> glm::mat4;
};
