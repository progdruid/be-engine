#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"
#include <sen-rhi/SenBackend.h>

#include "BePipelineBuilder.h"

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    const auto backbufferShader = BeAssetRegistry::GetShader("backbuffer").lock();
    auto scheme = BeAssetRegistry::GetMaterialScheme("backbuffer-material");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", scheme, false);
    _backbufferMaterial->SetTexture("InputTexture", InputTexture.lock());

    _pipeline = BePipelineBuilder::Start(*backbufferShader).SetColorFormats({ _renderer->GetSwapchainFormat() }).Build();
}

auto BeBackbufferPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetBindGroup(SubmissionBuffer.lock()->UniformMaterial.lock()->GetBindGroup(), 0);

    cmd.BeginPass(SenPassDesc{
        .ColorAttachments = {
            SenColorAttachment{ 
                .Texture = _renderer->GetBackbufferTexture(), 
                .LoadOp  = SenLoadOp::Clear, 
                .ClearColor = glm::vec4(ClearColor, 1.0f) 
            },
        },
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_backbufferMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    cmd.EndPass();
}

