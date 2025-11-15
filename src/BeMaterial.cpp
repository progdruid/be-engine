#include "BeMaterial.h"

#include <cassert>
#include <sstream>
#include <iomanip>

#include "BeShader.h"
#include "BeTexture.h"

BeMaterial::BeMaterial(std::string name, BeShader* shader, const ComPtr<ID3D11Device>& device)
: Name(std::move(name))
, Shader(shader) {
    assert(shader->HasMaterial);

    CalculateLayout();

    for (const auto& property : shader->MaterialTextureProperties) {
        std::shared_ptr<BeTexture> texture = nullptr;
        if (property.DefaultTexturePath == "white")
            texture = std::make_shared<BeTexture>(glm::vec4(1.f));
        else if (property.DefaultTexturePath == "black")
            texture = std::make_shared<BeTexture>(glm::vec4(0.f));
        texture->CreateSRV(device);
        _textures[property.Name] = {texture, property.SlotIndex};
    }
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    bufferDesc.ByteWidth = _bufferData.size() * sizeof(float);

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = _bufferData.data();

    Utils::Check << device->CreateBuffer(&bufferDesc, &data, _cbuffer.GetAddressOf());

    _cbufferDirty = false;
}

BeMaterial::~BeMaterial() = default;

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

auto BeMaterial::GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture> {
    assert(_textures.contains(propertyName));
    return _textures.at(propertyName).first;
}

auto BeMaterial::UpdateGPUBuffers(const ComPtr<ID3D11DeviceContext>& context) -> void {
    if (!_cbufferDirty) return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    Utils::Check << context->Map(_cbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, _bufferData.data(), _bufferData.size() * sizeof(float));
    context->Unmap(_cbuffer.Get(), 0);

    _cbufferDirty = false;
}

auto BeMaterial::CalculateLayout() -> void {
    uint32_t offsetBytes = 0;

    for (const auto& property : Shader->MaterialProperties) {
        constexpr uint32_t registerSizeBytes = 16;
        
        const uint32_t elementSizeBytes = BeMaterialPropertyDescriptor::SizeMap.at(property.PropertyType);
        const uint32_t positionInRegister = offsetBytes % registerSizeBytes;

        if (positionInRegister + elementSizeBytes > registerSizeBytes) {
            offsetBytes = ((offsetBytes / registerSizeBytes) + 1) * registerSizeBytes;
        }

        _propertyOffsets[property.Name] = offsetBytes / sizeof(float);
        offsetBytes += elementSizeBytes;
    }

    _bufferData.resize(offsetBytes / 4);
    for (const auto& property : Shader->MaterialProperties) {
        const uint32_t propertyOffset = _propertyOffsets.at(property.Name);
        const auto& defaultValue = property.DefaultValue;
        memcpy(_bufferData.data() + propertyOffset, defaultValue.data(), defaultValue.size() * sizeof(float));
    }
}

auto BeMaterial::Print() const -> std::string {
    std::stringstream ss;
    constexpr uint32_t FLOATS_PER_LINE = 4;

    for (size_t i = 0; i < _bufferData.size(); ++i) {
        ss << std::fixed << std::setprecision(1) << _bufferData[i] << "f ";
        if ((i + 1) % FLOATS_PER_LINE == 0) {
            ss << "\n";
        }
    }

    return ss.str();
}
