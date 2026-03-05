#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"
#include <sen-rhi/dx11/SenDx11Backend.h>

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    _backbufferShader = BeAssetRegistry::GetShader("backbuffer").lock();
    auto scheme = BeAssetRegistry::GetMaterialScheme("backbuffer-material");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", scheme, false, *_renderer);
    _backbufferMaterial->SetTexture("InputTexture", InputTexture.lock());

    // Create graphics pipeline
    auto pipelineDesc = _backbufferShader->CreatePipelineDesc();
    _pipeline = SenDx11Backend::Get().CreatePipeline(pipelineDesc);
}

auto BeBackbufferPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();

    // render target
    auto backbufferTarget = _renderer->GetBackbufferTarget();
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    context->ClearRenderTargetView(backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    context->OMSetRenderTargets(1, backbufferTarget.GetAddressOf(), nullptr);

    // bind pipeline (shaders + state)
    pipeline->BindPipeline(_pipeline);

    // bind material
    pipeline->BindMaterialAutomatic(_backbufferMaterial, _backbufferShader);

    // draw
    pipeline->Draw(4, 0);

    // cleanup
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    pipeline->ResetRenderState();
}

