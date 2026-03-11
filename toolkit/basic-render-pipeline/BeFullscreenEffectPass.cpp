#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
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
    if (Material) {
        _binding.Make(Material, Shader);
    }
    auto pipelineDesc = shader->GetPipelineDesc();
    for (const auto& tex : OutputTextures) {
        pipelineDesc.RenderTargetFormats.push_back(tex.lock()->Format);
    }
    if (Material) {
        pipelineDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout(), _binding.GetLayout() };
    } else {
        pipelineDesc.BindGroupLayouts = { _renderer->GetUniformBindGroupLayout() };
    }
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeFullscreenEffectPass: failed to create pipeline");
}

auto BeFullscreenEffectPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    // Build color attachments from output textures
    std::vector<SenColorAttachment> colorAttachments;
    for (const auto& tex : OutputTextures) {
        colorAttachments.push_back({ tex.lock()->Handle, 0, -1, SenLoadOp::Load });
    }

    cmd.BeginPass({
        .ColorAttachments = colorAttachments,
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetPipeline(_pipeline);

    if (Material) {
        cmd.SetBindGroup(_binding.Resolve(), 1);
    }

    cmd.Draw(4, 0);

    cmd.EndPass();
}
