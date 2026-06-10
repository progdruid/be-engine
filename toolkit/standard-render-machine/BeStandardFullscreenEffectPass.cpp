#include "BeStandardFullscreenEffectPass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>
#include <sen-rhi/SenTransitionBatch.h>

#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardFullscreenEffectPass::BeStandardFullscreenEffectPass(
    BeStandardRenderMachine* srm,
    std::weak_ptr<BeShader> shader,
    std::shared_ptr<BeMaterial> material,
    std::vector<std::shared_ptr<BeTexture>> outputs
) : _srm(srm), _shader(std::move(shader)), _material(std::move(material)), _outputs(std::move(outputs)) {}

auto BeStandardFullscreenEffectPass::Initialise() -> void {
    auto shader = _shader.lock();
    be_assert(shader, "BeStandardFullscreenEffectPass: shader not set");

    auto pipelineDesc = shader->GetPipelineDesc();
    for (const auto& tex : _outputs)
        pipelineDesc.RenderTargetFormats.push_back(tex->Format);
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeStandardFullscreenEffectPass: failed to create pipeline");
}

auto BeStandardFullscreenEffectPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    std::vector<SenColorAttachment> colorAttachments;
    colorAttachments.reserve(_outputs.size());
    for (const auto& tex : _outputs) {
        colorAttachments.push_back({ tex->Handle, 0, -1, SenLoadOp::Load });
    }
    
    if (_material) {
        SenTransitionBatch reads;
        for (const auto& [texture, slot] : _material->GetTextures()) {
            reads.Add(texture->Handle, SenResourceState::ShaderRead);
        }
        reads.TransitionAll(cmd);
    }

    cmd.BeginPass({
        .ColorAttachments = colorAttachments,
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    cmd.SetPipeline(_pipeline);
    if (_material)
        cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);

    cmd.EndPass();
}
