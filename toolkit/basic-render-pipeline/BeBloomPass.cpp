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
    //_brightMaterial->SetSampler("InputSampler", _renderer->GetPostProcessLinearClampSampler());

    _kawaseShader = BeAssetRegistry::GetShader("bloom-kawase").lock();
    auto kawaseScheme = BeAssetRegistry::GetMaterialScheme("bloom-kawase-material");;

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
        mat->UpdateGPUBuffers(_renderer->GetContext().Get());

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
        mat->UpdateGPUBuffers(_renderer->GetContext().Get());

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
    const auto context = _renderer->GetContext();
    const auto pipeline = _renderer->GetPipeline();
    
    const auto bloomMip0  = BloomMipTextures[0].lock();

    // targets
    context->ClearRenderTargetView(bloomMip0->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, bloomMip0->GetRTV().GetAddressOf(), nullptr);

    // shaders
    pipeline->BindShader(_brightShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_brightMaterial);
    
    // draw
    context->Draw(4, 0);

    // clear
    pipeline->Clear();
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    
}

auto BeBloomPass::RenderDownsamplePasses() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    uint32_t numberOfPreviousViewports = 1;
    D3D11_VIEWPORT previousViewport;
    context->RSGetViewports(&numberOfPreviousViewports, &previousViewport);

    pipeline->BindShader(_kawaseShader, BeShaderType::Vertex | BeShaderType::Pixel);
    
    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();

        // viewport
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(targetMip->Width);
        viewport.Height = static_cast<float>(targetMip->Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        // target
        context->ClearRenderTargetView(targetMip->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
        context->OMSetRenderTargets(1, targetMip->GetRTV().GetAddressOf(), nullptr);

        // material
        pipeline->BindMaterialAutomatic(_downsampleMaterials[mipTarget]);
        
        // draw
        context->Draw(4, 0);

        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    }

    pipeline->Clear();
    context->RSSetViewports(1, &previousViewport);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

auto BeBloomPass::RenderUpsamplePasses() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    uint32_t numberOfPreviousViewports = 1;
    D3D11_VIEWPORT previousViewport;
    context->RSGetViewports(&numberOfPreviousViewports, &previousViewport);
    
    // Set additive blend state for upsampling (accumulate into mips)
    ID3D11BlendState* additiveBlendState = nullptr;
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    Utils::Check << _renderer->GetDevice()->CreateBlendState(&blendDesc, &additiveBlendState);
    context->OMSetBlendState(additiveBlendState, nullptr, 0xFFFFFFFF);

    pipeline->BindShader(_kawaseShader, BeShaderType::Vertex | BeShaderType::Pixel);

    for (int32_t mipTarget = BloomMipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto targetMip = BloomMipTextures[mipTarget].lock();
        
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(targetMip->Width);
        viewport.Height = static_cast<float>(targetMip->Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);
        context->OMSetRenderTargets(1, targetMip->GetRTV().GetAddressOf(), nullptr);

        pipeline->BindMaterialAutomatic(_upsampleMaterials[mipTarget]);

        context->Draw(4, 0);

        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    }

    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    if (additiveBlendState) additiveBlendState->Release();

    pipeline->Clear();
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    context->RSSetViewports(1, &previousViewport);
}

auto BeBloomPass::RenderAddPass() const -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    const auto outputTexture = OutputTexture.lock();
    context->ClearRenderTargetView(outputTexture->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, outputTexture->GetRTV().GetAddressOf(), nullptr);

    pipeline->BindShader(_addShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic(_addMaterial);
    
    context->Draw(4, 0);

    pipeline->Clear();
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);   
}
