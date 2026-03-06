#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"
#include <sen-rhi/SenBackend.h>

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    _backbufferShader = BeAssetRegistry::GetShader("backbuffer").lock();
    auto scheme = BeAssetRegistry::GetMaterialScheme("backbuffer-material");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", scheme, false, *_renderer);
    _backbufferMaterial->SetTexture("InputTexture", InputTexture.lock());

    // Create graphics pipeline
    auto pipelineDesc = _backbufferShader->CreatePipelineDesc();
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
}

auto BeBackbufferPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    // Begin pass with backbuffer
    SenBackend::BeginPass({
        .ColorAttachments = {
            { _renderer->GetBackbufferTexture(), 0, -1, SenLoadOp::Clear, glm::vec4(ClearColor, 1.0f) },
        },
        .Viewport = _renderer->GetViewport(),
    });

    // bind pipeline (shaders + state)
    pipeline->BindPipeline(_pipeline);

    // bind material
    pipeline->BindMaterialAutomatic(_backbufferMaterial, _backbufferShader);

    // draw
    pipeline->Draw(4, 0);

    // End pass (automatically unbinds)
    SenBackend::EndPass();
}

