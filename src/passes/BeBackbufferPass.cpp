#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    const auto& device = _renderer->GetDevice();
    const auto& registry = _renderer->GetAssetRegistry().lock();

    _backbufferShader = BeShader::Create(device.Get(), "assets/shaders/backbuffer");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", false, _backbufferShader, _renderer->GetAssetRegistry().lock(), device);
    _backbufferMaterial->SetTexture("InputTexture", registry->GetTexture(InputTextureName).lock());
}

auto BeBackbufferPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    // render target
    const auto registry = _renderer->GetAssetRegistry().lock();
    auto backbufferTarget = _renderer->GetBackbufferTarget();
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    context->ClearRenderTargetView(backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    context->OMSetRenderTargets(1, backbufferTarget.GetAddressOf(), nullptr);

    // shaders
    pipeline->BindShader(_backbufferShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterial(_backbufferMaterial);
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    // draw
    context->Draw(4, 0);

    // clear
    pipeline->Clear();
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

