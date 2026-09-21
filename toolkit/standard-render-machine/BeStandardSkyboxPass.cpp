#include "BeStandardSkyboxPass.h"
#include <sen-rhi/Sen.h>

#include <umbrellas/include-libassert.h>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardSkyboxPass::BeStandardSkyboxPass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> depth,
    std::shared_ptr<BeTexture> envCubemap,
    std::shared_ptr<BeTexture> output
) : _srm(srm), _depth(std::move(depth)), _envCubemap(std::move(envCubemap)),
    _output(std::move(output)) {}

auto BeStandardSkyboxPass::Initialise(BeRenderer& renderer) -> void {

    _shader = BeShaderLibrary::GetShader("skybox");
    be_assert(_shader, "BeStandardSkyboxPass: skybox shader not found");

    const auto& scheme = BeShaderLibrary::GetShaderScheme(*_shader, "main");
    _material = BeMaterial::Create(scheme);
    _material->SetTexture("Depth", _depth);
    _material->SetTexture("EnvCubemap", _envCubemap);

    _state = BeDrawState::Create(*_shader).Build();
}

auto BeStandardSkyboxPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    _material->SetFloat1("ClampRadiance", _srm->Settings.Skybox.ClampRadiance);

    BePass pass(cmd);
    pass.UseTexture(_depth);
    pass.UseTexture(_envCubemap);
    pass.AddColorTarget(_output, SenLoadOp::Load);
    pass.SetViewport(_output->GetViewport());
    pass.Begin();
    pass.SetState(_state);
    pass.Bind("frame", *_srm->UniformMaterial);
    pass.Bind("main", *_material);

    pass.Draw(4);
    pass.End();
}
