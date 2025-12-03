#include "BeBloomPass.h"

#include "BeRenderer.h"
#include "BeAssetRegistry.h"
#include "BeTexture.h"
#include "BeShader.h"

BeBloomPass::BeBloomPass() = default;
BeBloomPass::~BeBloomPass() = default;

auto BeBloomPass::Initialise() -> void {
    const auto device = _renderer->GetDevice();

    _brightShader = BeShader::Create(device.Get(), "assets/shaders/BeBloomBright");
    _brightMaterial = BeMaterial::Create("Bright Pass Material", false, _brightShader, *AssetRegistry, device);

    _kawaseShader = BeShader::Create(device.Get(), "assets/shaders/BeBloomKawase");

    // Create downsample materials for each mip level (1 to size-1)
    _downsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        auto mat = BeMaterial::Create(
            "Downsample Mip " + std::to_string(mipTarget),
            false,
            _kawaseShader,
            *AssetRegistry,
            device
        );

        const auto sourceMip = _renderer->GetRenderResource(BloomMipTextureName+std::to_string(mipTarget - 1));

        const auto texelSizeX = 1.0f / sourceMip->Descriptor.CustomWidth;
        const auto texelSizeY = 1.0f / sourceMip->Descriptor.CustomHeight;

        const auto passRadius = 0.5f * (1 << (mipTarget - 1));

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", passRadius);
        mat->UpdateGPUBuffers(_renderer->GetContext().Get());

        _downsampleMaterials[mipTarget] = mat;
    }

    _upsampleMaterials.resize(BloomMipCount);
    for (uint32_t mipTarget = 0; mipTarget < BloomMipCount-1; ++mipTarget) {
        const auto mat = BeMaterial::Create(
            "Upsample Mip " + std::to_string(mipTarget),
            false,
            _kawaseShader,
            *AssetRegistry,
            device
        );

        const auto targetMip = _renderer->GetRenderResource(BloomMipTextureName + std::to_string(mipTarget));

        const auto texelSizeX = 1.0f / targetMip->Descriptor.CustomWidth;
        const auto texelSizeY = 1.0f / targetMip->Descriptor.CustomHeight;

        const auto upsampleRadius = 0.5f * (1 << mipTarget);

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", upsampleRadius);
        mat->UpdateGPUBuffers(_renderer->GetContext().Get());

        _upsampleMaterials[mipTarget] = mat;
    }

    _addShader = BeShader::Create(device.Get(), "assets/shaders/BeBloomAdd");    
}

auto BeBloomPass::Render() -> void {
    RenderBrightPass();
    RenderDownsamplePasses();
    RenderUpsamplePasses();
    RenderAddPass();
}

auto BeBloomPass::RenderBrightPass() const -> void {
    const auto context = _renderer->GetContext();

    const auto hdrTexture = _renderer->GetRenderResource(InputHDRTextureName);
    const auto bloomMip0 = _renderer->GetRenderResource(BloomMipTextureName + "0");

    // targets
    context->ClearRenderTargetView(bloomMip0->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, bloomMip0->GetRTV().GetAddressOf(), nullptr);

    // shaders
    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _brightShader->Bind(context.Get(), BeShaderType::Pixel);

    // resources
    context->PSSetShaderResources(0, 1, hdrTexture->GetSRV().GetAddressOf());
    context->PSSetSamplers(0, 1, _renderer->GetPostProcessLinearClampSampler().GetAddressOf());
    const auto& brightMaterialBuffer = _brightMaterial->GetBuffer();
    context->PSSetConstantBuffers(2, 1, brightMaterialBuffer.GetAddressOf());

    // draw
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // clear
    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, 1, Utils::NullSRVs);
    context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    
}

auto BeBloomPass::RenderDownsamplePasses() -> void {
    const auto context = _renderer->GetContext();

    uint32_t numberOfPreviousViewports = 1;
    D3D11_VIEWPORT previousViewport;
    context->RSGetViewports(&numberOfPreviousViewports, &previousViewport);

    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _kawaseShader->Bind(context.Get(), BeShaderType::Pixel);

    context->PSSetSamplers(0, 1, _renderer->GetPostProcessLinearClampSampler().GetAddressOf());

    for (uint32_t mipTarget = 1; mipTarget < BloomMipCount; ++mipTarget) {
        const auto sourceMip = _renderer->GetRenderResource(BloomMipTextureName + std::to_string(mipTarget - 1));
        const auto targetMip = _renderer->GetRenderResource(BloomMipTextureName + std::to_string(mipTarget));

        // sampler
        context->PSSetShaderResources(0, 1, sourceMip->GetSRV().GetAddressOf());

        // viewport
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(targetMip->Descriptor.CustomWidth);
        viewport.Height = static_cast<float>(targetMip->Descriptor.CustomHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        // target
        context->ClearRenderTargetView(targetMip->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
        context->OMSetRenderTargets(1, targetMip->GetRTV().GetAddressOf(), nullptr);

        // material
        const auto& downsampleMaterialBuffer = _downsampleMaterials[mipTarget]->GetBuffer();
        context->PSSetConstantBuffers(2, 1, downsampleMaterialBuffer.GetAddressOf());

        // draw
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);

        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    }

    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->PSSetShaderResources(0, 1, Utils::NullSRVs);
    context->RSSetViewports(1, &previousViewport);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

auto BeBloomPass::RenderUpsamplePasses() -> void {
    const auto context = _renderer->GetContext();

    uint32_t numberOfPreviousViewports = 1;
    D3D11_VIEWPORT previousViewport;
    context->RSGetViewports(&numberOfPreviousViewports, &previousViewport);

    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _kawaseShader->Bind(context.Get(), BeShaderType::Pixel);

    context->PSSetSamplers(0, 1, _renderer->GetPostProcessLinearClampSampler().GetAddressOf());

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

    for (int32_t mipTarget = BloomMipCount - 2; mipTarget >= 0; --mipTarget) {
        const auto sourceMip = _renderer->GetRenderResource(BloomMipTextureName + std::to_string(mipTarget + 1));
        const auto targetMip = _renderer->GetRenderResource(BloomMipTextureName + std::to_string(mipTarget));

        // Bind source (larger mip) for sampling
        context->PSSetShaderResources(0, 1, sourceMip->GetSRV().GetAddressOf());

        // Set render target and viewport
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(targetMip->Descriptor.CustomWidth);
        viewport.Height = static_cast<float>(targetMip->Descriptor.CustomHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        context->OMSetRenderTargets(1, targetMip->GetRTV().GetAddressOf(), nullptr);

        const auto& upsampleMaterialBuffer = _upsampleMaterials[mipTarget]->GetBuffer();
        context->PSSetConstantBuffers(2, 1, upsampleMaterialBuffer.GetAddressOf());

        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);

        context->PSSetShaderResources(0, 1, Utils::NullSRVs);
        context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    }

    // Restore blend state
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    if (additiveBlendState) additiveBlendState->Release();

    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, 1, Utils::NullSRVs);
    context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
    context->RSSetViewports(1, &previousViewport);
}

auto BeBloomPass::RenderAddPass() const -> void {
    const auto context = _renderer->GetContext();
    const auto hdrTexture = _renderer->GetRenderResource(InputHDRTextureName);
    const auto bloomMip0 = _renderer->GetRenderResource(BloomMipTextureName + "0");
    const auto dirtTexture = AssetRegistry->GetTexture(DirtTextureName);
    const auto outputTexture = _renderer->GetRenderResource(OutputTextureName);

    // targets
    context->ClearRenderTargetView(outputTexture->GetRTV().Get(), glm::value_ptr(glm::vec4(0.0f)));
    context->OMSetRenderTargets(1, outputTexture->GetRTV().GetAddressOf(), nullptr);

    // shaders
    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _addShader->Bind(context.Get(), BeShaderType::Pixel);

    // resources
    context->PSSetShaderResources(0, 1, hdrTexture->GetSRV().GetAddressOf());
    context->PSSetShaderResources(1, 1, bloomMip0->GetSRV().GetAddressOf());
    context->PSSetShaderResources(2, 1, dirtTexture.lock()->SRV.GetAddressOf());
    context->PSSetSamplers(0, 1, _renderer->GetPostProcessLinearClampSampler().GetAddressOf());

    // draw
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // clean
    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, 3, Utils::NullSRVs);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);   
}
