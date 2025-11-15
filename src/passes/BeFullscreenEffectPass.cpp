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
        inputResources.push_back(resource->SRV.Get());
    }
    context->PSSetShaderResources(0, inputResources.size(), inputResources.data());

    // Set output render targets
    std::vector<ID3D11RenderTargetView*> renderTargets;
    for (const auto& outputTextureName : OutputTextureNames) {
        const auto resource = _renderer->GetRenderResource(outputTextureName);
        renderTargets.push_back(resource->RTV.Get());
    }
    context->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), nullptr);

    
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    
    context->VSSetShader(_renderer->GetFullscreenVertexShader().Get(), nullptr, 0);
    context->PSSetShader(Shader->PixelShader.Get(), nullptr, 0);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);


    const std::vector<ID3D11ShaderResourceView*> emptyResources(InputTextureNames.size(), nullptr);
    context->PSSetShaderResources(0, emptyResources.size(), emptyResources.data());
    const std::vector<ID3D11RenderTargetView*> emptyTargets(OutputTextureNames.size(), nullptr);
    context->OMSetRenderTargets(emptyTargets.size(), emptyTargets.data(), nullptr);
    ID3D11SamplerState* emptySamplers[] = { nullptr };
    context->PSSetSamplers(0, 1, emptySamplers);
}
