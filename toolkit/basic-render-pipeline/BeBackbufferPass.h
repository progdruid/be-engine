#pragma once
#include <memory>
#include <string>
#include <umbrellas/include-glm.h>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeShader;
class BeBRPSubmissionBuffer;

class BeBackbufferPass final : public BeRenderPass {

    expose
    glm::vec3 ClearColor;
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;

    std::weak_ptr <BeTexture> InputTexture;

    hide
    std::shared_ptr<BeMaterial> _backbufferMaterial = nullptr;
    SenPipeline _pipeline;
    
    expose
    explicit BeBackbufferPass();
    ~BeBackbufferPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Composer Pass"; }
};
