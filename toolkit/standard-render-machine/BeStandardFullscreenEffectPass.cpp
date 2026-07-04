#include "BeStandardFullscreenEffectPass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BePass.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardFullscreenEffectPass::BeStandardFullscreenEffectPass(
    BeStandardRenderMachine* srm,
    raw_ptr<BeShader> shader,
    std::shared_ptr<BeMaterial> material,
    std::vector<std::shared_ptr<BeTexture>> outputs
) 
: _srm(srm)
, _shader(shader)
, _material(std::move(material))
, _outputs(std::move(outputs)) {}

auto BeStandardFullscreenEffectPass::Initialise() -> void {
    auto shader = _shader;
    be_assert(shader, "BeStandardFullscreenEffectPass: shader not set");

    auto pipelineDesc = shader->GetPipelineDesc();
    for (const auto& tex : _outputs) {
        pipelineDesc.RenderTargetFormats.push_back(tex->Format);
    }
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeStandardFullscreenEffectPass: failed to create pipeline");
}

auto BeStandardFullscreenEffectPass::Render(SenCommandBuffer& cmd) -> void {
    BePass pass(cmd);
    if (_material) {
        pass.UseMaterial(*_material);
    }
    pass.AddColorTargets(_outputs, SenLoadOp::Load);
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();

    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    cmd.SetPipeline(_pipeline);
    if (_material) {
        cmd.SetBindGroup(_material->GetBindGroup(), 1);
    }
    cmd.Draw(4, 0);

    pass.End();
}
