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

auto BePipeline::BindShader(const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void {
    assert(_boundShaderType == BeShaderType::None);
    assert(_boundShader == nullptr);
    assert(shader->Topology != SenTopology::Undefined);

    _context->IASetPrimitiveTopology(Sen::Dx11::ToTopology(shader->Topology));
    
    const auto boundType = shader->ShaderType & shaderType;
    
    if (HasAny(boundType, BeShaderType::Vertex)) {
        if (shader->ComputedInputLayout)
            _context->IASetInputLayout(shader->ComputedInputLayout.Get());
        _context->VSSetShader(shader->VertexShader.Get(), nullptr, 0);
    }
    if (HasAny(boundType, BeShaderType::Tesselation)) {
        _context->HSSetShader(shader->HullShader.Get(), nullptr, 0);
        _context->DSSetShader(shader->DomainShader.Get(), nullptr, 0);
    }
    if (HasAny(boundType, BeShaderType::Pixel)) {
        _context->PSSetShader(shader->PixelShader.Get(), nullptr, 0);
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
    const auto bufferHandle = material->GetBuffer();
    if (bufferHandle.IsValid()) {
        auto updated = material->UpdateGPUBuffers(_context);
        auto id = material->GetUniqueID();
        auto* rawBuffer = SenDx11Backend::Get().LookupBuffer(bufferHandle).Buffer.Get();

        if (HasAny(_boundShaderType, BeShaderType::Vertex) && (updated || _vertexCBufferIDCache[materialSlot] != id)) {
            _context->VSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _vertexCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && (updated || _tessCBufferIDCache[materialSlot] != id)) {
            _context->HSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _context->DSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _tessCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && (updated || _pixelCBufferIDCache[materialSlot] != id)) {
            _context->PSSetConstantBuffers(materialSlot, 1, &rawBuffer);
            _pixelCBufferIDCache[materialSlot] = id;
        }
    }

    BindMaterialTextures(*material);

    const auto& samplerSlots = material->GetSamplerPairs();
    for (auto& [handle, slot] : samplerSlots | std::views::values) {
        auto* rawSampler = SenDx11Backend::Get().LookupSampler(handle).Sampler.Get();
        if (HasAny(_boundShaderType, BeShaderType::Vertex) && _vertexSamplerCache[slot] != handle.id) {
            _context->VSSetSamplers(slot, 1, &rawSampler);
            _vertexSamplerCache[slot] = handle.id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && _tessSamplerCache[slot] != handle.id) {
            _context->HSSetSamplers(slot, 1, &rawSampler);
            _context->DSSetSamplers(slot, 1, &rawSampler);
            _tessSamplerCache[slot] = handle.id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && _pixelSamplerCache[slot] != handle.id) {
            _context->PSSetSamplers(slot, 1, &rawSampler);
            _pixelSamplerCache[slot] = handle.id;
        }
    }
}

auto BePipeline::Clear() -> void {
    assert(_boundShaderType != BeShaderType::None);
    assert(_boundShader != nullptr);

    _context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    
    _context->IASetInputLayout(nullptr);
    _context->VSSetShader(nullptr, nullptr, 0);
    _context->HSSetShader(nullptr, nullptr, 0);
    _context->DSSetShader(nullptr, nullptr, 0);
    _context->PSSetShader(nullptr, nullptr, 0);
    
    _boundShaderType = BeShaderType::None;
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
        const auto id  = texture->Handle.id;
        
        if (HasAny(_boundShaderType, BeShaderType::Vertex) && _vertexResCache[slot] != id) {
            _context->VSSetShaderResources(slot, 1, srv.GetAddressOf());
            _vertexResCache[slot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && _tessResCache[slot] != id) {
            _context->HSSetShaderResources(slot, 1, srv.GetAddressOf());
            _context->DSSetShaderResources(slot, 1, srv.GetAddressOf());
            _tessResCache[slot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && _pixelResCache[slot] != id) {
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
        auto rtv = renderTarget.lock()->GetRTV().Get();
        if (clearRTVs)
            _context->ClearRenderTargetView(rtv, glm::value_ptr(glm::vec4(0.0f)));
        rtvs.push_back(rtv);
    }

    //dsv
    ID3D11DepthStencilView* dsv = nullptr;
    if (depthTarget != nullptr) {
        dsv = depthTarget->GetDSV().Get();
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
        texture->GetRTV().Get(),
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
