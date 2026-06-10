#include "BeStandardBackbufferPass.h"

#include <sen-rhi/SenBackend.h>
#include <sen-rhi/SenTransitionBatch.h>

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipelineBuilder.h"
#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

BeStandardBackbufferPass::BeStandardBackbufferPass(
    BeStandardRenderMachine* srm,
    std::shared_ptr<BeTexture> input,
    glm::vec3 clearColor
) : _srm(srm), _input(std::move(input)), _clearColor(clearColor) {}

auto BeStandardBackbufferPass::Initialise() -> void {
    const auto shader = BeAssetRegistry::GetShader("backbuffer").lock();
    _material = BeMaterial::Create("backbuffer-material", false);
    _material->SetTexture("InputTexture", _input);
    _activeInput = _input;
    _pipeline = BePipelineBuilder::Start(*shader).SetColorFormats({ _renderer->GetSwapchainFormat() }).Build();
}

auto BeStandardBackbufferPass::Render() -> void {
    auto& cmd = _renderer->GetCommandBuffer();

    const auto debugTex = _srm->GetDebugChannelTexture();
    const auto& desired = debugTex ? debugTex : _input;
    if (desired != _activeInput) {
        _material->SetTexture("InputTexture", desired);
        _activeInput = desired;
    }

    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);

    SenTransitionBatch reads;
    for (const auto& [texture, slot] : _material->GetTextures()) {
        reads.Add(texture->Handle, SenResourceState::ShaderRead);
    }
    reads.TransitionAll(cmd);

    cmd.BeginPass({
        .ColorAttachments = { {
            .Texture    = _renderer->GetBackbufferTexture(),
            .LoadOp     = SenLoadOp::Clear,
            .ClearColor = glm::vec4(_clearColor, 1.0f),
        } },
        .Viewport = _renderer->GetViewport(),
    });
    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    cmd.EndPass();
}
