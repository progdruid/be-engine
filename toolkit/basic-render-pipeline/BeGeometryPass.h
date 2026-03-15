#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeBRPSubmissionBuffer;
class BeTexture;
class BeMaterial;
class BeShader;

class BeGeometryPass final : public BeRenderPass {

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

    std::weak_ptr<BeTexture> OutputTexture0;
    std::weak_ptr<BeTexture> OutputTexture1;
    std::weak_ptr<BeTexture> OutputTexture2;
    std::weak_ptr<BeTexture> OutputTexture3;
    std::weak_ptr<BeTexture> OutputDepthTexture;

    hide
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _objectMaterials;
    std::unordered_map<PipelineKey, SenPipeline, PipelineKeyHash> _shaderPipelines;

    expose
    explicit BeGeometryPass();
    ~BeGeometryPass() override;

    expose
    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Geometry Pass"; }
};
