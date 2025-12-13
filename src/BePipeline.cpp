#include "BePipeline.h"

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

auto BePipeline::BindMaterial(const std::shared_ptr<BeMaterial>& material) -> void {
    assert(_boundShaderType != BeShaderType::None); // Shader must be bound before material (see BindShader())
    assert(_boundShader != nullptr);                // Shader must be bound before material (see BindShader())
    assert(!material->Shader.owner_before(_boundShader) && !_boundShader.owner_before(material->Shader));
    // Material must use to the same shader (see BindShader())
    
    const auto& buffer = material->GetBuffer();
    if (buffer != nullptr) {
        material->UpdateGPUBuffers(_context);

        if (HasAny(_boundShaderType, BeShaderType::Vertex)) {
            _context->VSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation)) {
            _context->HSSetConstantBuffers(2, 1, buffer.GetAddressOf());
            _context->DSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel)) {
            _context->PSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
    }
    
    const auto& textureSlots = material->GetTexturePairs();
    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        if (HasAny(_boundShaderType, BeShaderType::Vertex)) {
            _context->VSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
        if (HasAny(_boundShaderType, BeShaderType::Tesselation)) {
            _context->HSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
            _context->DSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
        if (HasAny(_boundShaderType, BeShaderType::Pixel)) {
            _context->PSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
    }
    
    _boundMaterial = material;
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
    _boundMaterial = nullptr;
}
