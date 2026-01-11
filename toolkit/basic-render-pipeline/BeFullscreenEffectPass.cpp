#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {}

auto BeFullscreenEffectPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();
    const auto context = _renderer->GetContext();
    
    // render targets
    std::vector<ID3D11RenderTargetView*> renderTargets;
    for (const auto& outputTextureName : OutputTextureNames) {
        const auto resource = BeAssetRegistry::GetTexture(outputTextureName).lock();
        renderTargets.push_back(resource->GetRTV().Get());
    }
    context->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), nullptr);

    // shaders
    pipeline->BindShader(Shader.lock(), BeShaderType::Vertex | BeShaderType::Pixel);
    if (Material) {
        pipeline->BindMaterialAutomatic(Material);
    }

    // draw
    context->Draw(4, 0);

    // clear
    pipeline->Clear();
    context->OMSetRenderTargets(OutputTextureNames.size(), Utils::NullRTVs, nullptr);
}
