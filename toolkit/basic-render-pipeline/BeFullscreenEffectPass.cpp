#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeMaterial.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {
    auto shader = Shader.lock();
    be_assert(shader, "BeFullscreenEffectPass: shader not set");
    auto pipelineDesc = shader->GetPipelineDesc();
    for (const auto& tex : OutputTextures) {
        pipelineDesc.RenderTargetFormats.push_back(tex.lock()->Format);
    }
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeFullscreenEffectPass: failed to create pipeline");
}

auto BeFullscreenEffectPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    // Build color attachments from output textures
    std::vector<SenColorAttachment> colorAttachments;
    colorAttachments.reserve(OutputTextures.size());
    for (const auto& tex : OutputTextures) {
        colorAttachments.push_back({ tex.lock()->Handle, 0, -1, SenLoadOp::Load });
    }

    cmd.BeginPass({
        .ColorAttachments = colorAttachments,
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetBindGroup(SubmissionBuffer.lock()->UniformMaterial.lock()->GetBindGroup(), 0);
    cmd.SetPipeline(_pipeline);

    if (Material) {
        cmd.SetBindGroup(Material->GetBindGroup(), 1);
    }

    cmd.Draw(4, 0);

    cmd.EndPass();
}
