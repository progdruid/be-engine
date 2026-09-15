#include "BeStandardFullscreenEffectPass.h"

#include <umbrellas/include-libassert.h>

#include "BePass.h"
#include "BePipelineBuilder.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeRoot.h"
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
    auto shader = _shader;
    be_assert(shader, "BeStandardFullscreenEffectPass: shader not set");

    auto formats = std::vector<SenFormat>();
    for (const auto& tex : _outputs) {
        formats.push_back(tex->Format);
    }
    _pipeline = BePipelineBuilder::Start(*shader).SetColorFormats(formats).Build();
    be_assert(_pipeline.IsValid(), "BeStandardFullscreenEffectPass: failed to create pipeline");
}

auto BeStandardFullscreenEffectPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    BePass pass(cmd);
    if (_material) {
        pass.UseMaterial(*_material);
    }
    pass.AddColorTargets(_outputs, SenLoadOp::Load);
    pass.SetViewport(_outputs[0]->GetViewport());
    pass.Begin();

    cmd.SetPipeline(_pipeline);

    BeRoot root(*_shader);
    root.Use("frame", *_srm->UniformMaterial);
    if (_material) {
        root.Use("main", *_material);
    }
    root.Push(cmd);

    cmd.Draw(4, 0);

    pass.End();
}
