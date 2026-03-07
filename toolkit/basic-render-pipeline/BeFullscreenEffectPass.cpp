#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {
    auto shader = Shader.lock();
    be_assert(shader, "BeFullscreenEffectPass: shader not set");
    auto pipelineDesc = shader->CreatePipelineDesc();
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeFullscreenEffectPass: failed to create pipeline");
    if (Material) {
        _binding.Make(Material, Shader);
    }
}

auto BeFullscreenEffectPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    // Build color attachments from output textures
    std::vector<SenColorAttachment> colorAttachments;
    for (const auto& tex : OutputTextures) {
        colorAttachments.push_back({ tex.lock()->Handle, 0, -1, SenLoadOp::Load });
    }

    SenBackend::BeginPass({
        .ColorAttachments = colorAttachments,
        .Viewport = _renderer->GetViewport(),
    });

    pipeline->BindPipeline(_pipeline);

    if (Material) {
        pipeline->SetBindGroup(_binding.Resolve(), 1);
    }

    pipeline->Draw(4, 0);

    SenBackend::EndPass();
}
