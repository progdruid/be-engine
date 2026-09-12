#include "BeStandardFullscreenEffectPass.h"

#include <array>
#include <cstring>
#include <umbrellas/include-libassert.h>
#include <sen-rhi/SenBackend.h>

#include "BePass.h"
#include "BeMaterial.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardFullscreenEffectPass::BeStandardFullscreenEffectPass(
    BeStandardRenderMachine* srm,
    raw_ptr<BeShader> shader,
    std::shared_ptr<BeMaterial> material,
    std::vector<std::shared_ptr<BeTexture>> outputs
) 
: _srm(srm)
, _shader(shader)
, _material(std::move(material))
, _outputs(std::move(outputs)) {}

auto BeStandardFullscreenEffectPass::Initialise(BeRenderer& renderer) -> void {
    auto shader = _shader;
    be_assert(shader, "BeStandardFullscreenEffectPass: shader not set");

    auto pipelineDesc = shader->GetPipelineDesc();
    for (const auto& tex : _outputs) {
        pipelineDesc.RenderTargetFormats.push_back(tex->Format);
    }
    _pipeline = SenBackend::CreatePipeline(pipelineDesc);
    be_assert(_pipeline.IsValid(), "BeStandardFullscreenEffectPass: failed to create pipeline");
}

auto BeStandardFullscreenEffectPass::Render(BeRenderer& renderer, SenCommandBuffer& cmd) -> void {
    BePass pass(cmd);
    if (_material) {
        pass.UseMaterial(*_material);
    }
    pass.AddColorTargets(_outputs, SenLoadOp::Load);
    pass.SetViewport(_outputs[0]->GetViewport());
    pass.Begin();

    if (_shader->GetPipelineDesc().Bindless) {
        auto root = std::array<std::byte, SenMaxRootConstantSize>{};
        uint32_t cursor = 0;
        auto put64 = [&](uint64_t value) -> void { std::memcpy(root.data() + cursor, &value, 8); cursor += 8; };
        auto put32 = [&](uint32_t value) -> void { std::memcpy(root.data() + cursor, &value, 4); cursor += 4; };

        put64(_srm->UniformMaterial->GetCbufferAddress().Value);
        put64(_material ? _material->GetCbufferAddress().Value : 0);
        if (_material) {
            for (const auto index : _material->GetTextureHeapIndices()) { put32(index); }
            for (const auto index : _material->GetSamplerHeapIndices()) { put32(index); }
        }

        cmd.SetPipeline(_pipeline);
        cmd.PushRoot(root.data(), cursor);
        cmd.Draw(4, 0);
    } else {
        cmd.SetBindGroup(_srm->UniformMaterial->GetBindGroup(), 0);
        cmd.SetPipeline(_pipeline);
        if (_material) {
            cmd.SetBindGroup(_material->GetBindGroup(), 1);
        }
        cmd.Draw(4, 0);
    }

    pass.End();
}
