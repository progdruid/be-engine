#include "BeStandardBackbufferPass.h"

#include <sen-rhi/SenBackend.h>

#include "BeAssetRegistry.h"
#include "BePass.h"
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

    BePass pass;
    pass.UseMaterial(*_material);
    pass.AddColorTarget(_renderer->GetBackbufferTexture(), SenLoadOp::Clear, glm::vec4(_clearColor, 1.0f));
    pass.SetViewport(_renderer->GetViewport());
    pass.Begin();
    cmd.SetPipeline(_pipeline);
    cmd.SetBindGroup(_srm->UniformMaterial.lock()->GetBindGroup(), 0);
    cmd.SetBindGroup(_material->GetBindGroup(), 1);
    cmd.Draw(4, 0);
    pass.End();
}
