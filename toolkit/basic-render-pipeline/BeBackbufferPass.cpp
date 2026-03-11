#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
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

    _backbufferBinding.Make(_backbufferMaterial, _backbufferShader);
    auto pipelineDesc = _backbufferShader->GetPipelineDesc();
    pipelineDesc.RenderTargetFormats = { _renderer->GetSwapchainFormat() };
    pipelineDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout(), _backbufferBinding.GetLayout() };
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
}

auto BeBackbufferPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.BeginPass({
        .ColorAttachments = {
            { _renderer->GetBackbufferTexture(), 0, -1, SenLoadOp::Clear, glm::vec4(ClearColor, 1.0f) },
        },
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_backbufferBinding.Resolve(), 1);
    cmd.Draw(4, 0);
    cmd.EndPass();
}

