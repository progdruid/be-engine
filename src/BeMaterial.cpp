#include "BeMaterial.h"

#include <cassert>
#include <sstream>
#include <iomanip>

#include "BeShader.h"
#include "BeAssetRegistry.h"
#include "BeTexture.h"
#include "Utils.h"

auto BeMaterial::Create(
    std::string_view name,
    bool frequentlyUsed,
    std::weak_ptr<BeShader> shader,
    std::weak_ptr<BeAssetRegistry> registry,
    ComPtr<ID3D11Device> device
)
    -> std::shared_ptr<BeMaterial> {
    auto shaderLocked = shader.lock();
    assert(shaderLocked && "Shader must be valid");
    assert(shaderLocked->HasMaterial && "Shader must have material");

    auto material = std::make_shared<BeMaterial>(std::string(name), frequentlyUsed, shader, device);
    material->InitializeTextures(registry, device);
    return material;
}

auto BeMaterial::BindMaterial_Temporary(
    std::weak_ptr<BeMaterial> material,
    ComPtr<ID3D11DeviceContext> context,
    BeShaderType shaderType
) -> void {
    const auto& mat = material.lock();
    const auto& buffer = mat->GetBuffer();
    if (buffer != nullptr) {
        mat->UpdateGPUBuffers(context);

        if (HasAny(shaderType, BeShaderType::Vertex)) {
            context->VSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
        if (HasAny(shaderType, BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(2, 1, buffer.GetAddressOf());
            context->DSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
        if (HasAny(shaderType, BeShaderType::Pixel)) {
            context->PSSetConstantBuffers(2, 1, buffer.GetAddressOf());
        }
    }
    
    const auto& textureSlots = mat->GetTexturePairs();
    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        if (HasAny(shaderType, BeShaderType::Vertex)) {
            context->VSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
        if (HasAny(shaderType, BeShaderType::Tesselation)) {
            context->HSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
            context->DSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
        if (HasAny(shaderType, BeShaderType::Pixel)) {
            context->PSSetShaderResources(slot, 1, texture->GetSRV().GetAddressOf());
        }
    } 
}

auto BeMaterial::UnbindMaterial_Temporary(
    std::weak_ptr<BeMaterial> material,
    ComPtr<ID3D11DeviceContext> context,
    BeShaderType shaderType
) -> void {
    const auto& mat = material.lock();

    if (mat->GetBuffer() != nullptr) {
        if (HasAny(shaderType, BeShaderType::Vertex)) {
            context->VSSetConstantBuffers(2, 1, Utils::NullBuffers);
        }
        if (HasAny(shaderType, BeShaderType::Tesselation)) {
            context->HSSetConstantBuffers(2, 1, Utils::NullBuffers);
            context->DSSetConstantBuffers(2, 1, Utils::NullBuffers);
        }
        if (HasAny(shaderType, BeShaderType::Pixel)) {
            context->PSSetConstantBuffers(2, 1, Utils::NullBuffers);
        }
    }

    const auto& textureSlots = mat->GetTexturePairs();
    for (const auto& [texture, slot] : textureSlots | std::views::values) {
        if (HasAny(shaderType, BeShaderType::Vertex)) {
            context->VSSetShaderResources(slot, 1, Utils::NullSRVs);
        }
        if (HasAny(shaderType, BeShaderType::Tesselation)) {
            context->HSSetShaderResources(slot, 1, Utils::NullSRVs);
            context->DSSetShaderResources(slot, 1, Utils::NullSRVs);
        }
        if (HasAny(shaderType, BeShaderType::Pixel)) {
            context->PSSetShaderResources(slot, 1, Utils::NullSRVs);
        }
    } 
}

BeMaterial::BeMaterial(
    std::string name,
    const bool frequentlyUsed,
    const std::weak_ptr<BeShader>& shader,
    const ComPtr<ID3D11Device>& device
)
    : Name(std::move(name))
    , Shader(shader)
    , _isFrequentlyUsed(frequentlyUsed)
{
    const auto shaderLocked = Shader.lock();
    assert(shaderLocked);

    if (shaderLocked->MaterialProperties.size() == 0)
        return;
    
    AssembleData();

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    const uint32_t sizeInBytes = static_cast<uint32_t>(_bufferData.size() * sizeof(float));
    bufferDesc.ByteWidth = ((sizeInBytes + 15) / 16) * 16;
    if (_isFrequentlyUsed) {
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else {
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = _bufferData.data();

    Utils::Check << device->CreateBuffer(&bufferDesc, &data, _cbuffer.GetAddressOf());

    _cbufferDirty = false;
}

//BeMaterial::BeMaterial() = default;
BeMaterial::~BeMaterial() = default;

auto BeMaterial::InitializeTextures(std::weak_ptr<BeAssetRegistry> registry, ComPtr<ID3D11Device> device) -> void {
    auto shader = Shader.lock();
    assert(shader);
    
    for (const auto& property : shader->MaterialTextureProperties) {
        auto texWeak = registry.lock()->GetTexture(property.DefaultTexturePath);
        auto texture = texWeak.lock();
        assert(texture && ("Texture not found in registry: " + property.DefaultTexturePath).c_str());

        _textures[property.Name] = {texture, property.SlotIndex};
    }
}

auto BeMaterial::SetFloat(const std::string& propertyName, float value) -> void {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(float));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat2(const std::string& propertyName, glm::vec2 value) -> void {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec2));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat3(const std::string& propertyName, glm::vec3 value) -> void {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec3));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat4(const std::string& propertyName, glm::vec4 value) -> void {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec4));
    _cbufferDirty = true;
}

auto BeMaterial::SetMatrix(const std::string& propertyName, glm::mat4x4 value) -> void {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, glm::value_ptr(value), sizeof(glm::mat4x4));
    _cbufferDirty = true;
}

auto BeMaterial::SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void {
    assert(_textures.contains(propertyName));
    _textures.at(propertyName).first = texture;
}

auto BeMaterial::GetFloat(const std::string& propertyName) const -> float {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    float value;
    memcpy(&value, _bufferData.data() + offset, sizeof(float));
    return value;
}

auto BeMaterial::GetFloat2(const std::string& propertyName) const -> glm::vec2 {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec2 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec2));
    return value;
}

auto BeMaterial::GetFloat3(const std::string& propertyName) const -> glm::vec3 {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec3 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec3));
    return value;
}

auto BeMaterial::GetFloat4(const std::string& propertyName) const -> glm::vec4 {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec4 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec4));
    return value;
}

auto BeMaterial::GetMatrix(const std::string& propertyName) const -> glm::mat4x4 {
    assert(_propertyOffsets.contains(propertyName));
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::mat4x4 value;
    memcpy(glm::value_ptr(value), _bufferData.data() + offset, sizeof(glm::mat4x4));
    return value;
}

auto BeMaterial::GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture> {
    assert(_textures.contains(propertyName));
    return _textures.at(propertyName).first;
}

auto BeMaterial::UpdateGPUBuffers(const ComPtr<ID3D11DeviceContext>& context) -> void {
    if (!_cbufferDirty) return;

    if (_isFrequentlyUsed) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        Utils::Check << context->Map(_cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, _bufferData.data(), _bufferData.size() * sizeof(float));
        context->Unmap(_cbuffer.Get(), 0);
    }
    else {
        context->UpdateSubresource(_cbuffer.Get(), 0, nullptr, _bufferData.data(), 0, 0);
    }

    _cbufferDirty = false;
}

auto BeMaterial::AssembleData() -> void {
    auto shader = Shader.lock();
    assert(shader);

    uint32_t offsetBytes = 0;

    for (const auto& property : shader->MaterialProperties) {
        constexpr uint32_t registerSizeBytes = 16;

        const uint32_t elementSizeBytes = BeMaterialPropertyDescriptor::SizeMap.at(property.PropertyType);
        const uint32_t positionInRegister = offsetBytes % registerSizeBytes;
        
        if (positionInRegister + elementSizeBytes > registerSizeBytes && positionInRegister != 0) {
            offsetBytes = ((offsetBytes / registerSizeBytes) + 1) * registerSizeBytes;
        }

        _propertyOffsets[property.Name] = offsetBytes / sizeof(float);
        offsetBytes += elementSizeBytes;
    }

    _bufferData.resize(offsetBytes / 4);
    for (const auto& property : shader->MaterialProperties) {
        const uint32_t propertyOffset = _propertyOffsets.at(property.Name);
        const auto& defaultValue = property.DefaultValue;
        memcpy(_bufferData.data() + propertyOffset, defaultValue.data(), defaultValue.size() * sizeof(float));
    }
}

auto BeMaterial::Print() const -> std::string {
    std::stringstream ss;
    constexpr uint32_t FLOATS_PER_LINE = 4;

    for (size_t i = 0; i < _bufferData.size(); ++i) {
        ss << std::fixed << std::setprecision(3) << _bufferData[i] << "f ";
        if ((i + 1) % FLOATS_PER_LINE == 0) {
            ss << "\n";
        }
    }

    return ss.str();
}
