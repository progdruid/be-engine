#include "BeStandardBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardBackbufferPass::BeStandardBackbufferPass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> input,
    std::shared_ptr<BeTexture> depth
)
: _srm(srm)
, _input(std::move(input))
, _depth(std::move(depth)) {}

auto BeStandardBackbufferPass::Initialise(BeRenderer& renderer) -> void {
    const auto  shader = BeShaderLibrary::GetShader("backbuffer");
    const auto& scheme = shader->GetMaterialScheme("main");
    _material = BeMaterial::Create(scheme);
    _material->SetTexture("InputTexture", _input);
    _material->SetTexture("DepthTexture", _depth);
    _activeInput = _input;
    _pipeline = BePipelineBuilder::Start(*shader)
        .SetColorFormats({ renderer.GetSwapchainFormat() })
        .Build()
    ;
}

auto BeStandardBackbufferPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    _material->SetFloat1("DiscardFar", _srm->Settings.Backbuffer.DiscardFar ? 1.0f : 0.0f);

    BePass pass(cmd);
    pass.UseMaterial(*_material);
    pass.AddColorTarget(renderer.GetBackbufferTexture(), SenLoadOp::Clear, glm::vec4(_srm->Settings.Backbuffer.BackgroundColor, 1.0f));
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();
    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_srm->UniformMaterial->GetBindGroup(), 0);
    cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
