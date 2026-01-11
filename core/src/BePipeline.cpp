#include "BePipeline.h"

#include "BeAssetRegistry.h"
#include "BeMaterial.h"
#include "BeTexture.h"

auto BePipeline::Create(const ComPtr<ID3D11DeviceContext>& context)-> std::shared_ptr<BePipeline> {
    auto pipeline = std::shared_ptr<BePipeline>(new BePipeline());

    pipeline->_context = context;

    return pipeline;
}

auto BePipeline::BindShader(const std::shared_ptr<BeShader>& shader, BeShaderType shaderType) -> void {
    assert(_boundShaderType == BeShaderType::None);
    assert(_boundShader == nullptr);
    assert(shader->Topology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    _context->IASetPrimitiveTopology(shader->Topology);
    
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
    const auto& buffer = material->GetBuffer();
    if (buffer != nullptr) {
        auto updated = material->UpdateGPUBuffers(_context);
        auto id = material->GetUniqueID();
        
        if (HasAny(_boundShaderType, BeShaderType::Vertex) && (updated || _vertexCBufferIDCache[materialSlot] != id)) {
            _context->VSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _vertexCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && (updated || _tessCBufferIDCache[materialSlot] != id)) {
            _context->HSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _context->DSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _tessCBufferIDCache[materialSlot] = id;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && (updated || _pixelCBufferIDCache[materialSlot] != id)) {
            _context->PSSetConstantBuffers(materialSlot, 1, buffer.GetAddressOf());
            _pixelCBufferIDCache[materialSlot] = id;
        }
    }

    BindMaterialTextures(*material);

    const auto& samplerSlots = material->GetSamplerPairs();
    for (auto& [sampler, slot] : samplerSlots | std::views::values) {
        auto sampPtr = sampler.Get();
        if (HasAny(_boundShaderType, BeShaderType::Vertex) && _vertexSamplerCache[slot] != sampPtr) {
            _context->VSSetSamplers(slot, 1, sampler.GetAddressOf());
            _vertexSamplerCache[slot] = sampPtr;
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation) && _tessSamplerCache[slot] != sampPtr) {
            _context->HSSetSamplers(slot, 1, sampler.GetAddressOf());
            _context->DSSetSamplers(slot, 1, sampler.GetAddressOf());
            _tessSamplerCache[slot] = sampPtr;
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel) && _pixelSamplerCache[slot] != sampPtr) {
            _context->PSSetSamplers(slot, 1, sampler.GetAddressOf());
            _pixelSamplerCache[slot] = sampPtr;
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
}

auto BePipeline::BindMaterialTextures(const BeMaterial& material) -> void {
    
    const auto& textureSlots = material.GetTexturePairs();
    
    for (const auto& [texture, slot] : textureSlots | std::views::values) {

        const auto srv = texture->GetSRV();
        const auto id = texture->UniqueID;
        
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
