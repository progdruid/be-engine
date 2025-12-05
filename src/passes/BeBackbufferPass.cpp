#include "BeBackbufferPass.h"

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "BeShader.h"

BeBackbufferPass::BeBackbufferPass() = default;
BeBackbufferPass::~BeBackbufferPass() = default;

auto BeBackbufferPass::Initialise() -> void {
    const auto device = _renderer->GetDevice();

    _backbufferShader = BeShader::Create(device.Get(), "assets/shaders/backbuffer");
}

auto BeBackbufferPass::Render() -> void {
    const auto context = _renderer->GetContext();
    const auto registry = _renderer->GetAssetRegistry().lock();
    
    const auto inputResource = registry->GetTexture(InputTextureName).lock();
    auto backbufferTarget = _renderer->GetBackbufferTarget();

    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    context->ClearRenderTargetView(backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    context->OMSetRenderTargets(1, backbufferTarget.GetAddressOf(), nullptr);

    context->PSSetShaderResources(0, 1, inputResource->GetSRV().GetAddressOf());
    
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _backbufferShader->Bind(context.Get(), BeShaderType::Pixel);
    
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
    
    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, 4, Utils::NullSRVs);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

