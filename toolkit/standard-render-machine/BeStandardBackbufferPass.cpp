#include "BeStandardBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
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
    _shader = BeShaderLibrary::GetShader("backbuffer");
    const auto& scheme = BeShaderLibrary::GetShaderScheme(*_shader, "main");
    _material = BeMaterial::Create(scheme);
    _material->SetTexture("InputTexture", _input);
    _material->SetTexture("DepthTexture", _depth);
    _activeInput = _input;
    _state = BeDrawState::Create(*_shader).Build();
}

auto BeStandardBackbufferPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    _material->SetFloat1("DiscardFar", _srm->Settings.Backbuffer.DiscardFar ? 1.0f : 0.0f);

    BePass pass(cmd);
    pass.UseMaterial(*_material);
    pass.AddColorTarget(renderer.GetBackbufferView(), SenLoadOp::Clear, glm::vec4(_srm->Settings.Backbuffer.BackgroundColor, 1.0f));
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();
    pass.SetState(_state);
    pass.Bind("frame", *_srm->UniformMaterial);
    pass.Bind("main", *_material);

    pass.Draw(4);
    pass.End();
}
