#include "BeTestComputePass.h"

#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePass.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"

BeTestComputePass::BeTestComputePass(std::shared_ptr<BeTexture> input, std::shared_ptr<BeTexture> output)
    : _input(std::move(input)), _output(std::move(output)) {}

auto BeTestComputePass::Initialise() -> void {
    auto shader = BeAssetRegistry::GetShader("test-compute").lock();
    be_assert(shader, "BeTestComputePass: shader not found");

    _pipeline = SenBackend::CreatePipeline(shader->GetPipelineDesc());
    be_assert(_pipeline.IsValid(), "BeTestComputePass: failed to create pipeline");

    _material = BeMaterial::Create("test-compute-material", false);
    _material->SetTexture("ColorInput", _input);
    _material->SetTexture("Output", _output);
}

auto BeTestComputePass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    BePass pass;
    pass.SetCompute(true);
    pass.UseMaterial(*_material);
    pass.Begin();

    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_material->GetBindGroup(), 0);

    const uint32_t groupsX = (_output->Width  + 7) / 8;
    const uint32_t groupsY = (_output->Height + 7) / 8;
    cmd.Dispatch(groupsX, groupsY, 1);

    pass.End();
}
