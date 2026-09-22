#include "BeStandardFullscreenEffectPass.h"
#include <Sen.h>

#include <include-libassert.h>

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

auto BeStandardFullscreenEffectPass::Initialise(BeRenderer& renderer) -> void {
    be_assert(_shader, "BeStandardFullscreenEffectPass: shader not set");
    _state = BeDrawState::Create(*_shader).Build();
}

auto BeStandardFullscreenEffectPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    BePass pass(cmd);
    if (_material) {
        pass.UseMaterial(*_material);
    }
    pass.AddColorTargets(_outputs, SenLoadOp::Load);
    pass.SetViewport(_outputs[0]->GetViewport());
    pass.Begin();
    pass.SetState(_state);

    pass.Bind("frame", *_srm->UniformMaterial);
    if (_material) {
        pass.Bind("main", *_material);
    }

    pass.Draw(4);

    pass.End();
}
