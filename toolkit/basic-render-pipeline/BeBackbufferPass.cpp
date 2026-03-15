#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
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

    auto pipelineDesc = _backbufferShader->GetPipelineDesc();
    pipelineDesc.RenderTargetFormats = { _renderer->GetSwapchainFormat() };
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
}

auto BeBackbufferPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetBindGroup(SubmissionBuffer.lock()->UniformMaterial.lock()->GetBindGroup(), 0);

    cmd.BeginPass({
        .ColorAttachments = {
            { _renderer->GetBackbufferTexture(), 0, -1, SenLoadOp::Clear, glm::vec4(ClearColor, 1.0f) },
        },
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_backbufferMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    cmd.EndPass();
}

