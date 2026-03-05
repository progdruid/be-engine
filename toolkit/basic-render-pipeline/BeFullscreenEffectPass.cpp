#include "BeFullscreenEffectPass.h"

#include "BeAssetRegistry.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/dx11/SenDx11Backend.h>

BeFullscreenEffectPass::BeFullscreenEffectPass() = default;
BeFullscreenEffectPass::~BeFullscreenEffectPass() = default;

auto BeFullscreenEffectPass::Initialise() -> void {
    auto shader = Shader.lock();
    be_assert(shader, "BeFullscreenEffectPass: shader not set");
    auto pipelineDesc = shader->CreatePipelineDesc();
    _pipeline = SenDx11Backend::Get().CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeFullscreenEffectPass: failed to create pipeline");
}

auto BeFullscreenEffectPass::Render() -> void {
    const auto& pipeline = _renderer->GetPipeline();

    pipeline->BindTargets(OutputTextures, nullptr);
    pipeline->BindPipeline(_pipeline);

    if (Material) {
        pipeline->BindMaterialAutomatic(Material, Shader.lock());
    }

    pipeline->Draw(4, 0);
    pipeline->ResetRenderState();
}
