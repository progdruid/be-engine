#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BePipeline.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    _backbufferShader = BeAssetRegistry::GetShader("backbuffer").lock();
    auto scheme = BeAssetRegistry::GetMaterialScheme("backbuffer-material");
    _backbufferMaterial = BeMaterial::Create("Backbuffer Material", scheme, false, *_renderer);
    _backbufferMaterial->SetTexture("InputTexture", BeAssetRegistry::GetTexture(InputTextureName).lock());
    //_backbufferMaterial->SetSampler("InputSampler", _renderer->GetPointSampler());
}

auto BeBackbufferPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto& pipeline = _renderer->GetPipeline();
    
    // render target
    auto backbufferTarget = _renderer->GetBackbufferTarget();
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    context->ClearRenderTargetView(backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    context->OMSetRenderTargets(1, backbufferTarget.GetAddressOf(), nullptr);

    // shaders
    pipeline->BindShader(_backbufferShader, BeShaderType::Vertex | BeShaderType::Pixel);
    pipeline->BindMaterialAutomatic( _backbufferMaterial);

    // draw
    context->Draw(4, 0);

    // clear
    pipeline->Clear();
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

