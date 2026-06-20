#include "BeStandardEnvironmentBakePass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BePass.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardEnvironmentBakePass::BeStandardEnvironmentBakePass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> equirect,
    std::shared_ptr<BeTexture> envCubemap
) : _srm(srm), _equirect(std::move(equirect)), _envCubemap(std::move(envCubemap)) {}

auto BeStandardEnvironmentBakePass::Initialise() -> void {
    auto& registry = _srm->GetAssetRegistry();

    const auto shader = registry.GetShader("environment-bake").lock();
    be_assert(shader, "BeStandardEnvironmentBakePass: environment-bake shader not found");

    const auto& scheme = shader->GetMaterialScheme("main");
    for (uint32_t face = 0; face < FaceCount; ++face) {
        const auto mat = BeMaterial::Create(scheme, false);
        mat->SetFloat1("FaceIndex", static_cast<float>(face));
        mat->SetTexture("Equirect", _equirect);
        _faceMaterials[face] = mat;
    }

    _envPipeline = BePipelineBuilder::Start(*shader).SetColorFormats({ _envCubemap->Format }).Build();
}

auto BeStandardEnvironmentBakePass::Render(SenCommandBuffer& cmd) -> void {
    cmd.SetPipeline(_envPipeline);

    for (uint32_t face = 0; face < FaceCount; ++face) {
        BePass pass(cmd);
        pass.UseTexture(_equirect);
        pass.AddColorTarget(_envCubemap, SenLoadOp::DontCare, {}, 0, static_cast<int8_t>(face));
        pass.SetViewport(_envCubemap->GetMipViewport(0));
        pass.Begin();
        cmd.SetBindGroup(_faceMaterials[face]->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }
}
