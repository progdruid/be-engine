#include "BeStandardBloomPass.h"

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

BeStandardBloomPass::BeStandardBloomPass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> inputHDR,
    std::vector<std::shared_ptr<BeTexture>> mipTextures,
    std::shared_ptr<BeTexture> output,
    std::shared_ptr<BeTexture> dirtTexture,
    uint32_t mipCount
) : _srm(srm), _inputHDR(std::move(inputHDR)), _mipTextures(std::move(mipTextures)),
    _output(std::move(output)), _dirtTexture(std::move(dirtTexture)), _mipCount(mipCount) {}

auto BeStandardBloomPass::Initialise() -> void {
    const SenFormat mipFormat = _mipTextures[0]->Format;

    _brightMaterial = BeMaterial::Create("bloom-bright-material", false);
    _brightMaterial->SetTexture("HDRInput", _inputHDR);
    const auto brightShader = BeAssetRegistry::GetShader("bloom-bright").lock();
    be_assert(brightShader, "BeStandardBloomPass: bloom-bright shader not found");
    _brightPipeline = BePipelineBuilder::Start(*brightShader).SetColorFormats({ mipFormat }).Build();

    _downsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        auto mat = BeMaterial::Create("bloom-downsample-material", false);
        const auto sourceMip = _mipTextures[mipTarget - 1];
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / sourceMip->Width, 1.0f / sourceMip->Height));
        mat->SetFloat("UseKaris", mipTarget == 1 ? 1.0f : 0.0f);
        mat->SetTexture("BloomMipInput", sourceMip);
        _downsampleMaterials[mipTarget] = mat;
    }

    _upsampleMaterials.resize(_mipCount);
    for (uint32_t mipTarget = 0; mipTarget < _mipCount - 1; ++mipTarget) {
        auto mat = BeMaterial::Create("bloom-upsample-material", false);
        const auto sourceMip = _mipTextures[mipTarget + 1];
        mat->SetFloat2("TexelSize", glm::vec2(1.0f / sourceMip->Width, 1.0f / sourceMip->Height));
        mat->SetFloat("Radius", 1.0f);
        mat->SetTexture("BloomMipInput", sourceMip);
        _upsampleMaterials[mipTarget] = mat;
    }

    const auto downsampleShader = BeAssetRegistry::GetShader("bloom-downsample").lock();
    be_assert(downsampleShader, "BeStandardBloomPass: bloom-downsample shader not found");
    _downsamplePipeline = BePipelineBuilder::Start(*downsampleShader).SetColorFormats({ mipFormat }).Build();

    const auto upsampleShader = BeAssetRegistry::GetShader("bloom-upsample").lock();
    be_assert(upsampleShader, "BeStandardBloomPass: bloom-upsample shader not found");
    _upsamplePipeline = BePipelineBuilder::Start(*upsampleShader)
        .SetBlend({
            .Enable = true,
            .SrcBlend = SenBlendFactor::One, .DstBlend = SenBlendFactor::One, .BlendOp = SenBlendOp::Add,
            .SrcBlendAlpha = SenBlendFactor::Zero, .DstBlendAlpha = SenBlendFactor::One, .BlendOpAlpha = SenBlendOp::Add,
        })
        .SetColorFormats({ mipFormat }).Build();

    _addMaterial = BeMaterial::Create("bloom-add-material", false);
    _addMaterial->SetTexture("HDRInput",    _inputHDR);
    _addMaterial->SetTexture("BloomInput",  _mipTextures[0]);
    _addMaterial->SetTexture("DirtTexture", _dirtTexture);
    const auto addShader = BeAssetRegistry::GetShader("bloom-add").lock();
    be_assert(addShader, "BeStandardBloomPass: bloom-add shader not found");
    _addPipeline = BePipelineBuilder::Start(*addShader).SetColorFormats({ mipFormat }).Build();
}

auto BeStandardBloomPass::Render() -> void {
    _renderer->GetCommandBuffer().SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    RenderBrightPass();
    RenderDownsamplePasses();
    RenderUpsamplePasses();
    RenderAddPass();
}

auto BeStandardBloomPass::RenderBrightPass() -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto& mip0 = _mipTextures[0];

    BePass pass;
    pass.AddReadMaterial(*_brightMaterial);
    pass.AddColorTarget(mip0, SenLoadOp::Load);
    pass.SetViewport({ 0, 0, (float)mip0->Width, (float)mip0->Height, 0, 1 });
    pass.Begin();
    cmd.SetPipeline(_brightPipeline);
    cmd.SetBindGroup(_brightMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}

auto BeStandardBloomPass::RenderDownsamplePasses() -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    cmd.SetPipeline(_downsamplePipeline);
    for (uint32_t mipTarget = 1; mipTarget < _mipCount; ++mipTarget) {
        const auto& target = _mipTextures[mipTarget];

        BePass pass;
        pass.AddReadMaterial(*_downsampleMaterials[mipTarget]);
        pass.AddColorTarget(target, SenLoadOp::Load);
        pass.SetViewport({ 0, 0, (float)target->Width, (float)target->Height, 0, 1 });
        pass.Begin();
        cmd.SetBindGroup(_downsampleMaterials[mipTarget]->GetBindGroup(), 1);
        cmd.Draw(4, 0);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderUpsamplePasses() -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    cmd.SetPipeline(_upsamplePipeline);
    for (int32_t mipTarget = _mipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto& target = _mipTextures[mipTarget];

        BePass pass;
        pass.AddReadMaterial(*_upsampleMaterials[mipTarget]);
        pass.AddColorTarget(target, SenLoadOp::Load);
        pass.SetViewport({ 0, 0, (float)target->Width, (float)target->Height, 0, 1 });
        pass.Begin();
        cmd.SetBindGroup(_upsampleMaterials[mipTarget]->GetBindGroup(), 1);
        cmd.Draw(4, 0);
        pass.End();
    }
}

auto BeStandardBloomPass::RenderAddPass() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    BePass pass;
    pass.AddReadMaterial(*_addMaterial);
    pass.AddColorTarget(_output, SenLoadOp::Load);
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();
    cmd.SetPipeline(_addPipeline);
    cmd.SetBindGroup(_addMaterial->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
