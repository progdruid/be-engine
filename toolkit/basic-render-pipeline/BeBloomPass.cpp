#include "BeBloomPass.h"

#include "BeRenderer.h"
#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

BeBloomPass::BeBloomPass() = default;
BeBloomPass::~BeBloomPass() = default;

auto BeBloomPass::Initialise() -> void {
    _brightShader = BeAssetRegistry::GetShader("bloom-bright").lock();
    be_assert(_brightShader, "BeBloomPass: bloom-bright shader not found");
    auto brightScheme = BeAssetRegistry::GetMaterialScheme("bloom-bright-material");
    _brightMaterial = BeMaterial::Create("Bright Pass Material", brightScheme, false, *_renderer);
    _brightMaterial->SetTexture("HDRInput", InputHDRTexture.lock());

    // Create bright pass pipeline
    auto brightPipelineDesc = _brightShader->CreatePipelineDesc();
    _brightPipeline = SenBackend::CreatePipeline(brightPipelineDesc);
    be_assert(_brightPipeline.IsValid(), "BeBloomPass: failed to create bright pipeline");

    _kawaseShader = BeAssetRegistry::GetShader("bloom-kawase").lock();
    be_assert(_kawaseShader, "BeBloomPass: bloom-kawase shader not found");
    auto kawaseScheme = BeAssetRegistry::GetMaterialScheme("bloom-kawase-material");

    // Create downsample pipeline (normal blending)
    auto downsamplePipelineDesc = _kawaseShader->CreatePipelineDesc();
    _downsamplePipeline = SenBackend::CreatePipeline(downsamplePipelineDesc);
    be_assert(_downsamplePipeline.IsValid(), "BeBloomPass: failed to create downsample pipeline");

    // Create upsample pipeline (additive blending)
    auto upsamplePipelineDesc = _kawaseShader->CreatePipelineDesc();
    upsamplePipelineDesc.BlendState.Enable = true;
    upsamplePipelineDesc.BlendState.SrcBlend = SenBlendFactor::One;
    upsamplePipelineDesc.BlendState.DstBlend = SenBlendFactor::One;
    upsamplePipelineDesc.BlendState.BlendOp = SenBlendOp::Add;
    upsamplePipelineDesc.BlendState.SrcBlendAlpha = SenBlendFactor::Zero;
    upsamplePipelineDesc.BlendState.DstBlendAlpha = SenBlendFactor::One;
    upsamplePipelineDesc.BlendState.BlendOpAlpha = SenBlendOp::Add;
    _upsamplePipeline = SenBackend::CreatePipeline(upsamplePipelineDesc);
    be_assert(_upsamplePipeline.IsValid(), "BeBloomPass: failed to create upsample pipeline");

    // Create downsample materials for each mip level (1 to size-1)
    _downsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        auto mat = BeMaterial::Create(
            "Downsample Mip " + std::to_string(mipTarget),
            kawaseScheme,
            false,
            *_renderer
        );

        const auto sourceMip = BloomMipTextures[mipTarget - 1].lock();

        const auto texelSizeX = 1.0f / sourceMip->Width;
        const auto texelSizeY = 1.0f / sourceMip->Height;

        const auto passRadius = 0.5f * (1 << (mipTarget - 1));

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", passRadius);
        mat->SetTexture("BloomMipInput", sourceMip);
        mat->UpdateGPUBuffers();

        _downsampleMaterials[mipTarget] = mat;
    }

    _upsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 0; mipTarget < BloomMipCount-1; ++mipTarget) {
        const auto mat = BeMaterial::Create(
            "Upsample Mip " + std::to_string(mipTarget),
            kawaseScheme,
            false,
            *_renderer
        );

        const auto sourceMip = BloomMipTextures[mipTarget + 1].lock();
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        const auto texelSizeX = 1.0f / targetMip->Width;
        const auto texelSizeY = 1.0f / targetMip->Height;

        const auto upsampleRadius = 0.5f * (1 << mipTarget);

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", upsampleRadius);
        mat->SetTexture("BloomMipInput", sourceMip);
        mat->UpdateGPUBuffers();

        _upsampleMaterials[mipTarget] = mat;
    }

    _addShader = BeAssetRegistry::GetShader("bloom-add").lock();
    be_assert(_addShader, "BeBloomPass: bloom-add shader not found");
    const auto& addScheme = BeAssetRegistry::GetMaterialScheme("bloom-add-material");
    _addMaterial = BeMaterial::Create("Add Pass Material", addScheme, false, *_renderer);
    _addMaterial->SetTexture("HDRInput", InputHDRTexture.lock());
    _addMaterial->SetTexture("BloomInput", BloomMipTextures[0].lock());
    _addMaterial->SetTexture("DirtTexture", DirtTexture.lock());

    // Create add pipeline
    auto addPipelineDesc = _addShader->CreatePipelineDesc();
    _addPipeline = SenBackend::CreatePipeline(addPipelineDesc);
    be_assert(_addPipeline.IsValid(), "BeBloomPass: failed to create add pipeline");
}

auto BeBloomPass::Render() -> void {
    RenderBrightPass();
    RenderDownsamplePasses();
    RenderUpsamplePasses();
    RenderAddPass();
}

auto BeBloomPass::RenderBrightPass() const -> void {
    const auto pipeline = _renderer->GetPipeline();
    const auto bloomMip0 = BloomMipTextures[0].lock();

    SenBackend::BeginPass({
        .ColorAttachments = {
            { bloomMip0->Handle, 0, -1, SenLoadOp::Load },
        },
        .Viewport = { 0, 0, (float)bloomMip0->Width, (float)bloomMip0->Height, 0, 1 },
    });

    pipeline->BindPipeline(_brightPipeline);
    pipeline->BindMaterialAutomatic(_brightMaterial, _brightShader);
    pipeline->Draw(4, 0);

    SenBackend::EndPass();
}

auto BeBloomPass::RenderDownsamplePasses() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindPipeline(_downsamplePipeline);

    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        SenBackend::BeginPass({
            .ColorAttachments = {
                { targetMip->Handle, 0, -1, SenLoadOp::Load },
            },
            .Viewport = { 0, 0, (float)targetMip->Width, (float)targetMip->Height, 0, 1 },
        });

        pipeline->BindMaterialAutomatic(_downsampleMaterials[mipTarget], _kawaseShader);
        pipeline->Draw(4, 0);

        SenBackend::EndPass();
    }
}

auto BeBloomPass::RenderUpsamplePasses() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindPipeline(_upsamplePipeline);

    for (int32_t mipTarget = BloomMipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        SenBackend::BeginPass({
            .ColorAttachments = {
                { targetMip->Handle, 0, -1, SenLoadOp::Load },
            },
            .Viewport = { 0, 0, (float)targetMip->Width, (float)targetMip->Height, 0, 1 },
        });

        pipeline->BindMaterialAutomatic(_upsampleMaterials[mipTarget], _kawaseShader);
        pipeline->Draw(4, 0);

        SenBackend::EndPass();
    }
}

auto BeBloomPass::RenderAddPass() const -> void {
    const auto& pipeline = _renderer->GetPipeline();

    SenBackend::BeginPass({
        .ColorAttachments = {
            { OutputTexture.lock()->Handle, 0, -1, SenLoadOp::Load },
        },
        .Viewport = _renderer->GetViewport(),
    });

    pipeline->BindPipeline(_addPipeline);
    pipeline->BindMaterialAutomatic(_addMaterial, _addShader);
    pipeline->Draw(4, 0);

    SenBackend::EndPass();
}
