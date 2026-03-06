#include "BePipeline.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeTexture.h"
#include <sen-rhi/dx11/SenDx11Convert.h>
#include <sen-rhi/dx11/SenDx11Backend.h>

auto BePipeline::Create(const ComPtr<ID3D11DeviceContext>& context)-> std::shared_ptr<BePipeline> {
    auto pipeline = std::shared_ptr<BePipeline>(new BePipeline());

    pipeline->_context = context;

    return pipeline;
}

auto BePipeline::BindPipeline(SenPipeline pipeline) -> void {
    auto& entry = SenDx11Backend::Get().LookupPipeline(pipeline);

    // Cache bound shaders for material binding decisions
    // These are extracted from the pipeline desc passed to CreatePipeline
    // We need to look them up from the backend to get the handles back
    _boundVertexShader = entry.VertexShader;
    _boundHullShader = entry.HullShader;
    _boundDomainShader = entry.DomainShader;
    _boundPixelShader = entry.PixelShader;

    // Set topology
    _context->IASetPrimitiveTopology(entry.Topology);

    // Set vertex layout (optional for shaders using SV_VertexID)
    if (entry.InputLayout) {
        _context->IASetInputLayout(entry.InputLayout.Get());
    }

    // Set shaders
    if (entry.VertexShader) {
        _context->VSSetShader(entry.VertexShader.Get(), nullptr, 0);
    }
    if (entry.HullShader) {
        _context->HSSetShader(entry.HullShader.Get(), nullptr, 0);
    }
    if (entry.DomainShader) {
        _context->DSSetShader(entry.DomainShader.Get(), nullptr, 0);
    }
    if (entry.PixelShader) {
        _context->PSSetShader(entry.PixelShader.Get(), nullptr, 0);
    }

    // Set rasterizer state
    _context->RSSetState(entry.RasterizerState.Get());

    // Set blend state
    float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    _context->OMSetBlendState(entry.BlendState.Get(), blendFactor, 0xffffffff);

    // Set depth-stencil state
    _context->OMSetDepthStencilState(entry.DepthStencilState.Get(), 0);
}

auto BePipeline::BindMaterialAutomatic(const std::shared_ptr<BeMaterial>& material, const std::shared_ptr<BeShader>& shader) -> void {
    auto shaderToUse = shader ? shader : _boundShader;
    assert(shaderToUse);
    const uint8_t slot = shaderToUse->GetMaterialSlotByScheme(material->GetSchemeName());
    BindMaterialManual(material, slot);
}

auto BePipeline::BindMaterialManual(const std::shared_ptr<BeMaterial>& material, const uint8_t materialSlot) -> void {
    const auto bufferHandle = material->GetBuffer();
    if (bufferHandle.IsValid()) {
        auto updated = material->UpdateGPUBuffers();
        auto id = material->GetUniqueID();
        auto* rawBuffer = SenDx11Backend::Get().LookupBuffer(bufferHandle).Buffer.Get();

        if (_boundVertexShader && (updated || _vertexCBufferIDCache[materialSlot] != id)) {
            _context->VSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _vertexCBufferIDCache[materialSlot] = id;
        }
        if ((_boundHullShader && _boundDomainShader) && (updated || _tessCBufferIDCache[materialSlot] != id)) {
            _context->HSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _context->DSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _tessCBufferIDCache[materialSlot] = id;
        }
        if (_boundPixelShader && (updated || _pixelCBufferIDCache[materialSlot] != id)) {
            _context->PSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _pixelCBufferIDCache[materialSlot] = id;
        }
    }

    BindMaterialTextures(*material);

    const auto& samplerSlots = material->GetSamplerPairs();
    for (auto& [handle, slot] : samplerSlots | std::views::values) {
        auto* rawSampler = SenDx11Backend::Get().LookupSampler(handle).Sampler.Get();
        if (_boundVertexShader && _vertexSamplerCache[slot] != handle.ID) {
            _context->VSSetSamplers(slot, 1, &rawSampler);
            _vertexSamplerCache[slot] = handle.ID;
        }
        if ((_boundHullShader && _boundDomainShader) && _tessSamplerCache[slot] != handle.ID) {
            _context->HSSetSamplers(slot, 1, &rawSampler);
            _context->DSSetSamplers(slot, 1, &rawSampler);
            _tessSamplerCache[slot] = handle.ID;
        }
        if (_boundPixelShader && _pixelSamplerCache[slot] != handle.ID) {
            _context->PSSetSamplers(slot, 1, &rawSampler);
            _pixelSamplerCache[slot] = handle.ID;
        }
    }
}

auto BePipeline::Clear() -> void {
    assert(_boundShader != nullptr);

    _context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    _context->IASetInputLayout(nullptr);
    _context->VSSetShader(nullptr, nullptr, 0);
    _context->HSSetShader(nullptr, nullptr, 0);
    _context->DSSetShader(nullptr, nullptr, 0);
    _context->PSSetShader(nullptr, nullptr, 0);

    _boundVertexShader = nullptr;
    _boundHullShader = nullptr;
    _boundDomainShader = nullptr;
    _boundPixelShader = nullptr;
    _boundShader.reset();
    _boundShader = nullptr;
}

auto BePipeline::ClearCache() -> void {
    _vertexResCache.fill(0);
    _tessResCache.fill(0);
    _pixelResCache.fill(0);
    _vertexCBufferIDCache.fill(0);
    _tessCBufferIDCache.fill(0);
    _pixelCBufferIDCache.fill(0);
    _vertexSamplerCache.fill(0);
    _tessSamplerCache.fill(0);
    _pixelSamplerCache.fill(0);
}

auto BePipeline::BindMaterialTextures(const BeMaterial& material) -> void {
    
    const auto& textureSlots = material.GetTexturePairs();
    
    for (const auto& [texture, slot] : textureSlots | std::views::values) {

        const auto srv = SenDx11Backend::Get().LookupTexture(texture->Handle).SRV;
        const auto id  = texture->Handle.ID;
        
        if (_boundVertexShader && _vertexResCache[slot] != id) {
            _context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
            _vertexResCache[slot] = id;
        }
        if ((_boundHullShader && _boundDomainShader) && _tessResCache[slot] != id) {
            _context->HSSetShaderResources(slot, 1, srv.GetAddressOf());
            _context->DSSetShaderResources(slot, 1, srv.GetAddressOf());
            _tessResCache[slot] = id;
        }
        if (_boundPixelShader && _pixelResCache[slot] != id) {
            _context->PSSetShaderResources(slot, 1, srv.GetAddressOf());
            _pixelResCache[slot] = id;
        }
    }
}

auto BePipeline::BindTargets(
    const std::vector<std::weak_ptr<BeTexture>>& renderTargets,
    const BeTexture* depthTarget,
    bool clearRTVs
) const -> void {
    //rtvs
    std::vector<ID3D11RenderTargetView*> rtvs;
    rtvs.reserve(renderTargets.size());
    for (const auto& renderTarget : renderTargets) {
        auto texturePtr = renderTarget.lock();
        auto rtv = SenDx11Backend::Get().LookupTexture(texturePtr->Handle).MipRTVs[0].Get();
        if (clearRTVs)
            _context->ClearRenderTargetView(rtv, glm::value_ptr(glm::vec4(0.0f)));
        rtvs.push_back(rtv);
    }

    //dsv
    ID3D11DepthStencilView* dsv = nullptr;
    if (depthTarget != nullptr) {
        dsv = SenDx11Backend::Get().LookupTexture(depthTarget->Handle).DSV.Get();
        _context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    _context->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
}

auto BePipeline::ClearTargets() const -> void {
    _context->OMSetRenderTargets(0, nullptr, nullptr);
}

auto BePipeline::ResetTarget(const std::shared_ptr<BeTexture>& texture) const -> void {
    be_assert(
        HasAny(texture->Usage, SenTextureUsage::RenderTarget),
        "trying to reset a texture that is not a render target. "
        "only render targets can be reset. ",
        texture->Usage
    );

    _context->ClearRenderTargetView(
        SenDx11Backend::Get().LookupTexture(texture->Handle).MipRTVs[0].Get(),
        glm::value_ptr(glm::vec4(0.0f))
    );
}

auto BePipeline::BindVertexBuffer(SenBuffer buffer, uint32_t stride) const -> void {
    auto* raw = SenDx11Backend::Get().LookupBuffer(buffer).Buffer.Get();
    const UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, &raw, &stride, &offset);
}

auto BePipeline::BindIndexBuffer(SenBuffer buffer) const -> void {
    auto* raw = SenDx11Backend::Get().LookupBuffer(buffer).Buffer.Get();
    _context->IASetIndexBuffer(raw, DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::ClearVertexBuffer() const -> void {
    ID3D11Buffer* null = nullptr;
    const UINT zero = 0;
    _context->IASetVertexBuffers(0, 1, &null, &zero, &zero);
}

auto BePipeline::ClearIndexBuffer() const -> void {
    _context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
}

auto BePipeline::Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void {
    _context->Draw(vertexCount, startVertexLocation);
}

auto BePipeline::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void {
    _context->DrawIndexed(indexCount, startIndex, baseVertex);
}
