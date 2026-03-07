#include "BeBloomPass.h"

#include "BeRenderer.h"
#include "BeAssetRegistry.h"
#include "BeMaterial.h"
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
    _brightBinding.Make(_brightMaterial, _brightShader);

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
    _downsampleBindings.resize(BloomMipCount);
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

        _downsampleMaterials[mipTarget] = mat;
        _downsampleBindings[mipTarget].Make(mat, _kawaseShader);
    }

    _upsampleMaterials.resize(BloomMipCount);
    _upsampleBindings.resize(BloomMipCount);
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

        _upsampleMaterials[mipTarget] = mat;
        _upsampleBindings[mipTarget].Make(mat, _kawaseShader);
    }

    _addShader = BeAssetRegistry::GetShader("bloom-add").lock();
    be_assert(_addShader, "BeBloomPass: bloom-add shader not found");
    const auto& addScheme = BeAssetRegistry::GetMaterialScheme("bloom-add-material");
    _addMaterial = BeMaterial::Create("Add Pass Material", addScheme, false, *_renderer);
    _addMaterial->SetTexture("HDRInput", InputHDRTexture.lock());
    _addMaterial->SetTexture("BloomInput", BloomMipTextures[0].lock());
    _addMaterial->SetTexture("DirtTexture", DirtTexture.lock());
    _addBinding.Make(_addMaterial, _addShader);

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

auto BeBloomPass::RenderBrightPass() -> void {
    auto& cmd = _renderer->GetCommandBuffer();
    const auto bloomMip0 = BloomMipTextures[0].lock();

    cmd.BeginPass({
        .ColorAttachments = {
            { bloomMip0->Handle, 0, -1, SenLoadOp::Load },
        },
        .Viewport = { 0, 0, (float)bloomMip0->Width, (float)bloomMip0->Height, 0, 1 },
    });

    cmd.SetPipeline(_brightPipeline);
    cmd.SetBindGroup(_brightBinding.Resolve(), 1);
    cmd.Draw(4, 0);

    cmd.EndPass();
}

auto BeBloomPass::RenderDownsamplePasses() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetPipeline(_downsamplePipeline);

    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        cmd.BeginPass({
            .ColorAttachments = {
                { targetMip->Handle, 0, -1, SenLoadOp::Load },
            },
            .Viewport = { 0, 0, (float)targetMip->Width, (float)targetMip->Height, 0, 1 },
        });

        cmd.SetBindGroup(_downsampleBindings[mipTarget].Resolve(), 1);
        cmd.Draw(4, 0);

        cmd.EndPass();
    }
}

auto BeBloomPass::RenderUpsamplePasses() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.SetPipeline(_upsamplePipeline);

    for (int32_t mipTarget = BloomMipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        cmd.BeginPass({
            .ColorAttachments = {
                { targetMip->Handle, 0, -1, SenLoadOp::Load },
            },
            .Viewport = { 0, 0, (float)targetMip->Width, (float)targetMip->Height, 0, 1 },
        });

        cmd.SetBindGroup(_upsampleBindings[mipTarget].Resolve(), 1);
        cmd.Draw(4, 0);

        cmd.EndPass();
    }
}

auto BeBloomPass::RenderAddPass() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    cmd.BeginPass({
        .ColorAttachments = {
            { OutputTexture.lock()->Handle, 0, -1, SenLoadOp::Load },
        },
        .Viewport = _renderer->GetViewport(),
    });

    cmd.SetPipeline(_addPipeline);
    cmd.SetBindGroup(_addBinding.Resolve(), 1);
    cmd.Draw(4, 0);

    cmd.EndPass();
}
