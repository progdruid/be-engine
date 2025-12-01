#include "BeFullscreenEffectPass.h"

#include "BeRenderer.h"
#include "BeShader.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {}

auto BeFullscreenEffectPass::Render() -> void {
    const auto context = _renderer->GetContext();

    // Set input resources
    std::vector<ID3D11ShaderResourceView*> inputResources;
    for (const auto& inputTextureName : InputTextureNames) {
        const auto resource = _renderer->GetRenderResource(inputTextureName);
        inputResources.push_back(resource->GetSRV().Get());
    }
    context->PSSetShaderResources(0, inputResources.size(), inputResources.data());

    // Set output render targets
    std::vector<ID3D11RenderTargetView*> renderTargets;
    for (const auto& outputTextureName : OutputTextureNames) {
        const auto resource = _renderer->GetRenderResource(outputTextureName);
        renderTargets.push_back(resource->GetRTV().Get());
    }
    context->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), nullptr);
    
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    Shader->Bind(context.Get(), BeShaderType::Pixel);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);


    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, InputTextureNames.size(), Utils::NullSRVs);
    context->OMSetRenderTargets(OutputTextureNames.size(), Utils::NullRTVs, nullptr);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
}
