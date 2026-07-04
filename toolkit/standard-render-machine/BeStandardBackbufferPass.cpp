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
    glm::vec3 clearColor
) 
: _srm(srm)
, _input(std::move(input))
, _clearColor(clearColor) {}

auto BeStandardBackbufferPass::Initialise() -> void {
    const auto  shader = BeShaderLibrary::GetShader("backbuffer");
    const auto& scheme = shader->GetMaterialScheme("main");
    _material = BeMaterial::Create(scheme, false);
    _material->SetTexture("InputTexture", _input);
    _activeInput = _input;
    _pipeline = BePipelineBuilder::Start(*shader)
        .SetColorFormats({ _renderer->GetSwapchainFormat() })
        .Build()
    ;
}

auto BeStandardBackbufferPass::Render(SenCommandBuffer& cmd) -> void {
    BePass pass(cmd);
    pass.UseMaterial(*_material);
    pass.AddColorTarget(_renderer->GetBackbufferTexture(), SenLoadOp::Clear, glm::vec4(_clearColor, 1.0f));
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();
    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
