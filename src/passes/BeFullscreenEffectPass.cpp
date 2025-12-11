#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeShader.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {}

auto BeFullscreenEffectPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto registry = _renderer->GetAssetRegistry().lock();
    
    // render targets
    std::vector<ID3D11RenderTargetView*> renderTargets;
    for (const auto& outputTextureName : OutputTextureNames) {
        const auto resource = registry->GetTexture(outputTextureName).lock();
        renderTargets.push_back(resource->GetRTV().Get());
    }
    context->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), nullptr);

    // shaders
    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    Shader->Bind(context.Get(), BeShaderType::Pixel);

    // input
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    if (Material) {
        BeMaterial::BindMaterial_Temporary(Material, context, BeShaderType::All);
    }

    // draw
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);

    // clear
    BeShader::Unbind(context.Get(), BeShaderType::All);
    BeMaterial::UnbindMaterial_Temporary(Material, context, BeShaderType::All);
    context->OMSetRenderTargets(OutputTextureNames.size(), Utils::NullRTVs, nullptr);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
}
