#include "BeBloomPass.h"

#include "BeRenderer.h"
#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeShader.h"
#include "BeTexture.h"

BeBloomPass::BeBloomPass() = default;
BeBloomPass::~BeBloomPass() = default;

auto BeBloomPass::Initialise() -> void {
    _brightShader = BeAssetRegistry::GetShader("bloom-bright").lock();
    auto brightScheme = BeAssetRegistry::GetMaterialScheme("bloom-bright-material");
    _brightMaterial = BeMaterial::Create("Bright Pass Material", brightScheme, false, *_renderer);
    _brightMaterial->SetTexture("HDRInput", InputHDRTexture.lock());

    _kawaseShader = BeAssetRegistry::GetShader("bloom-kawase").lock();
    auto kawaseScheme = BeAssetRegistry::GetMaterialScheme("bloom-kawase-material");

    _downsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        auto mat = BeMaterial::Create(
            "Downsample Mip " + std::to_string(mipTarget),
            kawaseScheme, false, *_renderer
        );

        const auto sourceMip = BloomMipTextures[mipTarget - 1].lock();
        const auto texelSizeX = 1.0f / sourceMip->Width;
        const auto texelSizeY = 1.0f / sourceMip->Height;
        const auto passRadius = 0.5f * (1 << (mipTarget - 1));

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", passRadius);
        mat->SetTexture("BloomMipInput", sourceMip);
        _renderer->GetPipeline()->UpdateMaterialBuffers(mat);

        _downsampleMaterials[mipTarget] = mat;
    }

    _upsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 0; mipTarget < BloomMipCount - 1; ++mipTarget) {
        const auto mat = BeMaterial::Create(
            "Upsample Mip " + std::to_string(mipTarget),
            kawaseScheme, false, *_renderer
        );

        const auto sourceMip = BloomMipTextures[mipTarget + 1].lock();
        const auto targetMip = BloomMipTextures[mipTarget].lock();
        const auto texelSizeX = 1.0f / targetMip->Width;
        const auto texelSizeY = 1.0f / targetMip->Height;
        const auto upsampleRadius = 0.5f * (1 << mipTarget);

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", upsampleRadius);
        mat->SetTexture("BloomMipInput", sourceMip);
        _renderer->GetPipeline()->UpdateMaterialBuffers(mat);

        _upsampleMaterials[mipTarget] = mat;
    }

    _addShader = BeAssetRegistry::GetShader("bloom-add").lock();
    const auto& addScheme = BeAssetRegistry::GetMaterialScheme("bloom-add-material");
    _addMaterial = BeMaterial::Create("Add Pass Material", addScheme, false, *_renderer);
    _addMaterial->SetTexture("HDRInput", InputHDRTexture.lock());
    _addMaterial->SetTexture("BloomInput", BloomMipTextures[0].lock());
    _addMaterial->SetTexture("DirtTexture", DirtTexture.lock());
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

    pipeline->BindTargets({ bloomMip0 }, nullptr, false);
    pipeline->BindShader(_brightShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_brightMaterial);
    pipeline->Draw(4, 0);

    pipeline->Clear();
    pipeline->ClearTargets();
}

auto BeBloomPass::RenderDownsamplePasses() -> void {
    const auto& pipeline = _renderer->GetPipeline();
    const auto previousViewport = pipeline->GetViewport();

    pipeline->BindShader(_kawaseShader, BeShaderType::Vertex | BeShaderType::Pixel);

    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        BeViewport viewport;
        viewport.Width = static_cast<float>(targetMip->Width);
        viewport.Height = static_cast<float>(targetMip->Height);
        pipeline->SetViewport(viewport);

        pipeline->BindTargets({ targetMip }, nullptr, false);
        pipeline->BindMaterialAutomatic(_downsampleMaterials[mipTarget]);

        pipeline->Draw(4, 0);

        pipeline->ClearTargets();
    }

    pipeline->Clear();
    pipeline->SetViewport(previousViewport);
}

auto BeBloomPass::RenderUpsamplePasses() -> void {
    const auto& pipeline = _renderer->GetPipeline();
    const auto previousViewport = pipeline->GetViewport();

    pipeline->SetAdditiveBlending();
    pipeline->BindShader(_kawaseShader, BeShaderType::Vertex | BeShaderType::Pixel);

    for (int32_t mipTarget = BloomMipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        BeViewport viewport;
        viewport.Width = static_cast<float>(targetMip->Width);
        viewport.Height = static_cast<float>(targetMip->Height);
        pipeline->SetViewport(viewport);

        pipeline->BindTargets({ targetMip }, nullptr, false);
        pipeline->BindMaterialAutomatic(_upsampleMaterials[mipTarget]);

        pipeline->Draw(4, 0);

        pipeline->ClearTargets();
    }

    pipeline->ClearBlendState();
    pipeline->Clear();
    pipeline->SetViewport(previousViewport);
}

auto BeBloomPass::RenderAddPass() const -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindTargets({ OutputTexture }, nullptr, false);
    pipeline->BindShader(_addShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_addMaterial);

    pipeline->Draw(4, 0);

    pipeline->Clear();
    pipeline->ClearTargets();
}
