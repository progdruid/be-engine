#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {}

auto BeFullscreenEffectPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();
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
    pipeline->BindShader(Shader, BeShaderType::Vertex | BeShaderType::Pixel);
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());
    if (Material) {
        pipeline->BindMaterial(Material);
    }

    // draw
    context->Draw(4, 0);

    // clear
    pipeline->Clear();
    context->OMSetRenderTargets(OutputTextureNames.size(), Utils::NullRTVs, nullptr);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
}
