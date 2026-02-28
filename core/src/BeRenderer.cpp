#include "BeRenderer.h"

#include "BeRenderPass.h"

auto BeRenderer::AddRenderPass(BeRenderPass* renderPass) -> void {
    _passes.push_back(renderPass);
    renderPass->InjectRenderer(this);
}

auto BeRenderer::ClearPasses() -> void {
    for (auto pass : _passes) {
        delete pass;
    }
    _passes.clear();
}

auto BeRenderer::InitialisePasses() const -> void {
    for (const auto& pass : _passes)
        pass->Initialise();
}
