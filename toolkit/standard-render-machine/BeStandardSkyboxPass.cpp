#include "BeStandardSkyboxPass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
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

    const auto shader = BeShaderLibrary::GetShader("skybox");
    be_assert(shader, "BeStandardSkyboxPass: skybox shader not found");

    const auto& scheme = shader->GetMaterialScheme("main");
    _material = BeMaterial::Create(scheme, false);
    _material->SetTexture("Depth", _depth);
    _material->SetTexture("EnvCubemap", _envCubemap);

    _pipeline = BePipelineBuilder::Start(*shader).SetColorFormats({ _output->Format }).Build();
}

auto BeStandardSkyboxPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    _material->SetFloat1("ClampRadiance", _srm->Settings.Skybox.ClampRadiance);

    cmd.SetBindGroup(_srm->UniformMaterial->GetBindGroup(), 0);

    BePass pass(cmd);
    pass.UseTexture(_depth);
    pass.UseTexture(_envCubemap);
    pass.AddColorTarget(_output, SenLoadOp::Load);
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();
    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
