#include "BePipeline.h"
#include "BePipelineImpl.h"
#include "BeRendererImpl.h"
#include "BeTextureImpl.h"
#include "BeShaderImpl.h"
#include "DxUtils.h"

#include "BeRenderer.h"
#include "BeShader.h"
#include "BeTexture.h"
#include "BeMaterial.h"
#include "BeMaterialImpl.h"
#include "BeModel.h"
#include "BeAssetRegistry.h"
#include "BeBRPSubmissionBuffer.h"
#include "BeBRPSubmissionBufferImpl.h"

#include <cassert>
#include <umbrellas/include-glm.h>
#include <umbrellas/include-libassert.h>

struct BeSamplerImpl {
    ComPtr<ID3D11SamplerState> samplerState;
};

BePipeline::BePipeline() = default;
BePipeline::~BePipeline() = default;

auto BePipeline::Create(BeRenderer& renderer) -> std::shared_ptr<BePipeline> {
    auto pipeline = std::shared_ptr<BePipeline>(new BePipeline());
    pipeline->_impl = std::make_unique<BePipelineImpl>();
    pipeline->_impl->context = renderer.GetPlatformImpl()->context;
    pipeline->_impl->device = renderer.GetPlatformImpl()->device;
    pipeline->_impl->rendererImpl = renderer.GetPlatformImpl();
    return pipeline;
}

static auto TopologyToD3D(BeTopology t) -> D3D11_PRIMITIVE_TOPOLOGY {
    switch (t) {
        case BeTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case BeTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case BeTopology::PatchList3:    return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:                        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

auto BePipeline::BindShader(const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void {
    assert(_boundShaderType == BeShaderType::None);
    assert(_boundShader == nullptr);
    assert(shader->Topology != BeTopology::Undefined);

    auto& ctx = _impl->context;
    auto* si = shader->GetPlatformImpl();

    ctx->IASetPrimitiveTopology(TopologyToD3D(shader->Topology));

    const auto boundType = shader->ShaderType & shaderType;

    if (HasAny(boundType, BeShaderType::Vertex)) {
        if (si->computedInputLayout)
            ctx->IASetInputLayout(si->computedInputLayout.Get());
        ctx->VSSetShader(si->vertexShader.Get(), nullptr, 0);
    }
    if (HasAny(boundType, BeShaderType::Tesselation)) {
        ctx->HSSetShader(si->hullShader.Get(), nullptr, 0);
        ctx->DSSetShader(si->domainShader.Get(), nullptr, 0);
    }
    if (HasAny(boundType, BeShaderType::Pixel)) {
        ctx->PSSetShader(si->pixelShader.Get(), nullptr, 0);
    }

    _boundShaderType = boundType;
    _boundShader = shader;
}

auto BePipeline::BindMaterialAutomatic(const std::shared_ptr<BeMaterial>& material) -> void {
    assert(_boundShader);
    const uint8_t slot = _boundShader->GetMaterialSlotByScheme(material->GetSchemeName());
    BindMaterialManual(material, slot);
}

auto BePipeline::BindMaterialManual(const std::shared_ptr<BeMaterial>& material, const uint8_t materialSlot) -> void {
    auto& ctx = _impl->context;

    auto* matImpl = material->GetPlatformImpl();
    const auto& buffer = matImpl ? matImpl->cbuffer : ComPtr<ID3D11Buffer>();
    if (buffer != nullptr) {
        auto updated = material->UpdatePlatformBuffer();
        auto id = material->GetUniqueID();

        if (HasAny(_boundShaderType, BeShaderType::Vertex) && (updated || _impl->vertexCBufferIDCache[materialSlot] != id)) {
            ctx->VSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _impl->vertexCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && (updated || _impl->tessCBufferIDCache[materialSlot] != id)) {
            ctx->HSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            ctx->DSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _impl->tessCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && (updated || _impl->pixelCBufferIDCache[materialSlot] != id)) {
            ctx->PSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _impl->pixelCBufferIDCache[materialSlot] = id;
        }
    }

    BindMaterialTextures(*material);

    const auto& samplerSlots = material->GetSamplerPairsInternal();
    for (auto& [sampler, slot] : samplerSlots | std::views::values) {
        auto sampPtr = sampler->samplerState.Get();
        if (HasAny(_boundShaderType, BeShaderType::Vertex) && _impl->vertexSamplerCache[slot] != sampPtr) {
            ctx->VSSetSamplers(slot, 1, sampler->samplerState.GetAddressOf());
            _impl->vertexSamplerCache[slot] = sampPtr;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && _impl->tessSamplerCache[slot] != sampPtr) {
            ctx->HSSetSamplers(slot, 1, sampler->samplerState.GetAddressOf());
            ctx->DSSetSamplers(slot, 1, sampler->samplerState.GetAddressOf());
            _impl->tessSamplerCache[slot] = sampPtr;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && _impl->pixelSamplerCache[slot] != sampPtr) {
            ctx->PSSetSamplers(slot, 1, sampler->samplerState.GetAddressOf());
            _impl->pixelSamplerCache[slot] = sampPtr;
        }
    }
}

auto BePipeline::Clear() -> void {
    assert(_boundShaderType != BeShaderType::None);
    assert(_boundShader != nullptr);

    auto& ctx = _impl->context;
    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    ctx->IASetInputLayout(nullptr);
    ctx->VSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);

    _boundShaderType = BeShaderType::None;
    _boundShader.reset();
    _boundShader = nullptr;
}

auto BePipeline::ClearCache() -> void {
    _impl->vertexResCache.fill(0);
    _impl->tessResCache.fill(0);
    _impl->pixelResCache.fill(0);
    _impl->vertexCBufferIDCache.fill(0);
    _impl->tessCBufferIDCache.fill(0);
    _impl->pixelCBufferIDCache.fill(0);
}

auto BePipeline::BindMaterialTextures(const BeMaterial& material) -> void {
    auto& ctx = _impl->context;
    const auto& textureSlots = material.GetTexturePairs();

    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        auto* texImpl = texture->GetPlatformImpl();
        const auto srv = texImpl->srv;
        const auto id = texture->UniqueID;

        if (HasAny(_boundShaderType, BeShaderType::Vertex) && _impl->vertexResCache[slot] != id) {
            ctx->VSSetShaderResources(slot, 1, srv.GetAddressOf());
            _impl->vertexResCache[slot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && _impl->tessResCache[slot] != id) {
            ctx->HSSetShaderResources(slot, 1, srv.GetAddressOf());
            ctx->DSSetShaderResources(slot, 1, srv.GetAddressOf());
            _impl->tessResCache[slot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && _impl->pixelResCache[slot] != id) {
            ctx->PSSetShaderResources(slot, 1, srv.GetAddressOf());
            _impl->pixelResCache[slot] = id;
        }
    }
}

auto BePipeline::BindTargets(
    const std::vector<std::weak_ptr<BeTexture>>& renderTargets,
    const BeTexture* depthTarget,
    bool clearRTVs
) const -> void {
    auto& ctx = _impl->context;
    std::vector<ID3D11RenderTargetView*> rtvs;
    rtvs.reserve(renderTargets.size());
    for (const auto& renderTarget : renderTargets) {
        auto* texImpl = renderTarget.lock()->GetPlatformImpl();
        auto rtv = texImpl->mipRTVs[0].Get();
        if (clearRTVs)
            ctx->ClearRenderTargetView(rtv, glm::value_ptr(glm::vec4(0.0f)));
        rtvs.push_back(rtv);
    }

    ID3D11DepthStencilView* dsv = nullptr;
    if (depthTarget != nullptr) {
        dsv = depthTarget->GetPlatformImpl()->dsv.Get();
        ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    ctx->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.data(), dsv);
}

auto BePipeline::ClearTargets() const -> void {
    _impl->context->OMSetRenderTargets(0, nullptr, nullptr);
}

auto BePipeline::ResetTarget(const std::shared_ptr<BeTexture>& texture) const -> void {
    be_assert(
        HasAny(texture->BindFlags, BeBindFlags::RenderTarget),
        "trying to reset a texture that is not a render target."
    );
    auto* texImpl = texture->GetPlatformImpl();
    _impl->context->ClearRenderTargetView(
        texImpl->mipRTVs[0].Get(),
        glm::value_ptr(glm::vec4(0.0f))
    );
}

auto BePipeline::Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void {
    _impl->context->Draw(vertexCount, startVertexLocation);
}

auto BePipeline::DrawSlice(const BeDrawSlice& slice) const -> void {
    _impl->context->DrawIndexed(slice.IndexCount, slice.StartIndexLocation, slice.BaseVertexLocation);
}

auto BePipeline::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void {
    _impl->context->DrawIndexed(indexCount, startIndex, baseVertex);
}

auto BePipeline::SetViewport(const BeViewport& viewport) const -> void {
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = viewport.TopLeftX;
    vp.TopLeftY = viewport.TopLeftY;
    vp.Width = viewport.Width;
    vp.Height = viewport.Height;
    vp.MinDepth = viewport.MinDepth;
    vp.MaxDepth = viewport.MaxDepth;
    _impl->context->RSSetViewports(1, &vp);
}

auto BePipeline::GetViewport() const -> BeViewport {
    UINT numViewports = 1;
    D3D11_VIEWPORT dxVp = {};
    _impl->context->RSGetViewports(&numViewports, &dxVp);
    BeViewport vp;
    vp.TopLeftX = dxVp.TopLeftX;
    vp.TopLeftY = dxVp.TopLeftY;
    vp.Width = dxVp.Width;
    vp.Height = dxVp.Height;
    vp.MinDepth = dxVp.MinDepth;
    vp.MaxDepth = dxVp.MaxDepth;
    return vp;
}

auto BePipeline::SetCullMode(BeCullMode mode) const -> void {
    auto* ri = _impl->rendererImpl;
    switch (mode) {
        case BeCullMode::None:
            _impl->context->RSSetState(ri->rasterizerCullNone.Get());
            break;
        case BeCullMode::Back:
        default:
            _impl->context->RSSetState(ri->rasterizerCullBack.Get());
            break;
    }
}

auto BePipeline::BindMeshBuffers(const BeBRPSubmissionBuffer& submissionBuffer) const -> void {
    auto& ctx = _impl->context;
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    auto* bufImpl = submissionBuffer.GetPlatformImpl();
    auto vb = bufImpl->sharedVertexBuffer;
    auto ib = bufImpl->sharedIndexBuffer;
    ctx->IASetVertexBuffers(0, 1, vb.GetAddressOf(), &stride, &offset);
    ctx->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::UnbindMeshBuffers() const -> void {
    auto& ctx = _impl->context;
    uint32_t stride = sizeof(BeFullVertex);
    uint32_t offset = 0;
    ctx->IASetVertexBuffers(0, 1, DxUtils::NullBuffers, &stride, &offset);
    ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::SetDepthOnlyTarget(BeTexture* depthTexture) const -> void {
    auto* texImpl = depthTexture->GetPlatformImpl();
    _impl->context->OMSetRenderTargets(0, DxUtils::NullRTVs, texImpl->dsv.Get());
}

auto BePipeline::SetCubemapDepthTarget(BeTexture* cubemapTexture, uint32_t face) const -> void {
    auto* texImpl = cubemapTexture->GetPlatformImpl();
    auto cubemapDSV = texImpl->cubemapDSVs[face];
    _impl->context->OMSetRenderTargets(0, nullptr, cubemapDSV.Get());
}

auto BePipeline::ClearDepthTarget(BeTexture* depthTexture) const -> void {
    auto* texImpl = depthTexture->GetPlatformImpl();
    if (texImpl->dsv) {
        _impl->context->ClearDepthStencilView(texImpl->dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
}

auto BePipeline::BindBackbuffer(const glm::vec4& clearColor) const -> void {
    auto& ctx = _impl->context;
    auto& backbuffer = _impl->rendererImpl->backbufferTarget;
    ctx->ClearRenderTargetView(backbuffer.Get(), glm::value_ptr(clearColor));
    ctx->OMSetRenderTargets(1, backbuffer.GetAddressOf(), nullptr);
}

auto BePipeline::UnbindBackbuffer() const -> void {
    _impl->context->OMSetRenderTargets(0, DxUtils::NullRTVs, nullptr);
}

auto BePipeline::SetAdditiveBlending() const -> void {
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ComPtr<ID3D11BlendState> blendState;
    DxUtils::Check << _impl->device->CreateBlendState(&blendDesc, blendState.GetAddressOf());
    _impl->context->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);
}

auto BePipeline::ClearBlendState() const -> void {
    _impl->context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

auto BePipeline::UpdateMaterialBuffers(const std::shared_ptr<BeMaterial>& material) const -> void {
    material->UpdatePlatformBuffer();
}
