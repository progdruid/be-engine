#include "BeFullscreenEffectPass.h"

#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {}

auto BeFullscreenEffectPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindTargets(OutputTextures, nullptr);
    pipeline->BindShader(Shader.lock(), BeShaderType::Vertex | BeShaderType::Pixel);
    if (Material) { pipeline->BindMaterialAutomatic(Material); }
    pipeline->Draw(4, 0);
    pipeline->Clear();
}
