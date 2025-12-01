#include "BeBloomPass.h"

#include "BeRenderer.h"
#include "BeShader.h"

BeBloomPass::BeBloomPass() = default;
BeBloomPass::~BeBloomPass() = default;

auto BeBloomPass::Initialise() -> void {
    const auto device = _renderer->GetDevice();

    _brightShader = BeShader::Create(device.Get(), "assets/shaders/BeBloomBright");
    _brightMaterial = BeMaterial::Create("Bright Pass Material", false, _brightShader, *AssetRegistry, device);

    _downsampleShader = BeShader::Create(device.Get(), "assets/shaders/BeBloomDownsample");

    // Create downsample materials for each mip level (1 to size-1)
    for (uint32_t mipTarget = 1; mipTarget < BloomMipTextureNames.size(); ++mipTarget) {
        auto mat = BeMaterial::Create(
            "Downsample Mip " + std::to_string(mipTarget),
            false,
            _downsampleShader,
            *AssetRegistry,
            device
        );

        const auto sourceMip = _renderer->GetRenderResource(BloomMipTextureNames[mipTarget - 1]);

        const auto texelSizeX = 1.0f / sourceMip->Descriptor.CustomWidth;
        const auto texelSizeY = 1.0f / sourceMip->Descriptor.CustomHeight;

        const auto passRadius = 0.5f * (1 << (mipTarget - 1));

        mat->SetFloat2("TexelSize", glm::vec2(texelSizeX, texelSizeY));
        mat->SetFloat("PassRadius", passRadius);
        mat->SetFloat("MipLevel", 0.0f);
        mat->UpdateGPUBuffers(_renderer->GetContext().Get());

        _downsampleMaterials.push_back(mat);
    }

    //_upsampleShader = BeShader::Create(device.Get(), "assets/shaders/bloomUpsample");
}

auto BeBloomPass::Render() -> void {
    RenderBrightPass();
    RenderDownsamplePasses();
    //RenderUpsamplePasses();
}

auto BeBloomPass::RenderBrightPass() const -> void {
    const auto context = _renderer->GetContext();

    const auto hdrTexture = _renderer->GetRenderResource(InputHDRTextureName);
    const auto bloomMip0 = _renderer->GetRenderResource(BloomMipTextureNames[0]);

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
    _downsampleShader->Bind(context.Get(), BeShaderType::Pixel);

    context->PSSetSamplers(0, 1, _renderer->GetPostProcessLinearClampSampler().GetAddressOf());

    for (uint32_t mipTarget = 1; mipTarget < BloomMipTextureNames.size(); ++mipTarget) {
        const auto sourceMip = _renderer->GetRenderResource(BloomMipTextureNames[mipTarget - 1]);
        const auto targetMip = _renderer->GetRenderResource(BloomMipTextureNames[mipTarget]);

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
        const auto& downsampleMaterialBuffer = _downsampleMaterials[mipTarget - 1]->GetBuffer();
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




