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
    std::shared_ptr<BeTexture> envCubemap,
    std::shared_ptr<BeTexture> irradianceCubemap
) : _srm(srm), _equirect(std::move(equirect)), _envCubemap(std::move(envCubemap)),
    _irradianceCubemap(std::move(irradianceCubemap)) {}

auto BeStandardEnvironmentBakePass::Initialise() -> void {
    auto& registry = _srm->GetAssetRegistry();

    const auto envShader = registry.GetShader("environment-bake").lock();
    be_assert(envShader, "BeStandardEnvironmentBakePass: environment-bake shader not found");

    const auto& envScheme = envShader->GetMaterialScheme("main");
    for (uint32_t face = 0; face < FaceCount; ++face) {
        const auto mat = BeMaterial::Create(envScheme, false);
        mat->SetFloat1("FaceIndex", static_cast<float>(face));
        mat->SetTexture("Equirect", _equirect);
        _envFaceMaterials[face] = mat;
    }
    _envPipeline = BePipelineBuilder::Start(*envShader).SetColorFormats({ _envCubemap->Format }).Build();

    const auto irradianceShader = registry.GetShader("irradiance-bake").lock();
    be_assert(irradianceShader, "BeStandardEnvironmentBakePass: irradiance-bake shader not found");

    const auto& irradianceScheme = irradianceShader->GetMaterialScheme("main");
    for (uint32_t face = 0; face < FaceCount; ++face) {
        const auto mat = BeMaterial::Create(irradianceScheme, false);
        mat->SetFloat1("FaceIndex", static_cast<float>(face));
        mat->SetTexture("EnvCubemap", _envCubemap);
        _irradianceFaceMaterials[face] = mat;
    }
    _irradiancePipeline = BePipelineBuilder::Start(*irradianceShader).SetColorFormats({ _irradianceCubemap->Format }).Build();
}

auto BeStandardEnvironmentBakePass::Render(SenCommandBuffer& cmd) -> void {
    cmd.SetPipeline(_envPipeline);
    for (uint32_t face = 0; face < FaceCount; ++face) {
        BePass pass(cmd);
        pass.UseTexture(_equirect);
        pass.AddColorTarget(_envCubemap, SenLoadOp::DontCare, {}, 0, static_cast<int8_t>(face));
        pass.SetViewport(_envCubemap->GetMipViewport(0));
        pass.Begin();
        cmd.SetBindGroup(_envFaceMaterials[face]->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }

    cmd.SetPipeline(_irradiancePipeline);
    for (uint32_t face = 0; face < FaceCount; ++face) {
        BePass pass(cmd);
        pass.UseTexture(_envCubemap);
        pass.AddColorTarget(_irradianceCubemap, SenLoadOp::DontCare, {}, 0, static_cast<int8_t>(face));
        pass.SetViewport(_irradianceCubemap->GetMipViewport(0));
        pass.Begin();
        cmd.SetBindGroup(_irradianceFaceMaterials[face]->GetBindGroup(), 0);
        cmd.Draw(4, 0);
        pass.End();
    }
}
