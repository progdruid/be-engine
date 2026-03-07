#pragma once

#include <memory>
#include <unordered_map>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"
#include "BeMaterialBinding.h"

class BeBRPSubmissionBuffer;
class BeTexture;
class BeMaterial;
class BeShader;

class BeGeometryPass final : public BeRenderPass {
    
    expose
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;
    
    std::weak_ptr<BeTexture> OutputTexture0;
    std::weak_ptr<BeTexture> OutputTexture1;
    std::weak_ptr<BeTexture> OutputTexture2;
    std::weak_ptr<BeTexture> OutputTexture3;
    std::weak_ptr<BeTexture> OutputDepthTexture;

    hide
    std::shared_ptr<BeMaterial> _objectMaterial;
    std::unordered_map<BeShader*, SenPipeline> _shaderPipelines;
    std::unordered_map<BeShader*, BeMaterialBinding> _objectBindings;
    
    expose
    explicit BeGeometryPass();
    ~BeGeometryPass() override;

    expose
    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Geometry Pass"; }
};
