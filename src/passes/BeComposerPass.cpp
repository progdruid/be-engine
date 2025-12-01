#include "BeComposerPass.h"

#include "BeRenderer.h"
#include "BeRenderResource.h"
#include "BeShader.h"

BeComposerPass::BeComposerPass() = default;
BeComposerPass::~BeComposerPass() = default;

auto BeComposerPass::Initialise() -> void {
    const auto device = _renderer->GetDevice();

    _composerShader = BeShader::Create(device.Get(), "assets/shaders/composer");
}

auto BeComposerPass::Render() -> void {
    const auto context = _renderer->GetContext();

    BeRenderResource* depthResource    = _renderer->GetRenderResource(InputDepthTextureName);
    BeRenderResource* gbufferResource0 = _renderer->GetRenderResource(InputTexture0Name);
    BeRenderResource* gbufferResource1 = _renderer->GetRenderResource(InputTexture1Name);
    BeRenderResource* gbufferResource2 = _renderer->GetRenderResource(InputTexture2Name);
    BeRenderResource* lightingResource = _renderer->GetRenderResource(InputLightTextureName);

    auto backbufferTarget = _renderer->GetBackbufferTarget();
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    context->ClearRenderTargetView(backbufferTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    context->OMSetRenderTargets(1, backbufferTarget.GetAddressOf(), nullptr);

    context->PSSetShaderResources(0, 1, depthResource->GetSRV().GetAddressOf());
    context->PSSetShaderResources(1, 1, gbufferResource0->GetSRV().GetAddressOf());
    context->PSSetShaderResources(2, 1, gbufferResource1->GetSRV().GetAddressOf());
    context->PSSetShaderResources(3, 1, gbufferResource2->GetSRV().GetAddressOf());
    context->PSSetShaderResources(4, 1, lightingResource->GetSRV().GetAddressOf());
    
    context->PSSetSamplers(0, 1, _renderer->GetPointSampler().GetAddressOf());

    _renderer->GetFullscreenVertexShader()->Bind(context.Get(), BeShaderType::Vertex);
    _composerShader->Bind(context.Get(), BeShaderType::Pixel);
    
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
    
    BeShader::Unbind(context.Get(), BeShaderType::All);
    context->PSSetShaderResources(0, 4, Utils::NullSRVs);
    context->PSSetSamplers(0, 1, Utils::NullSamplers);
    context->OMSetRenderTargets(1, Utils::NullRTVs, nullptr);
}

