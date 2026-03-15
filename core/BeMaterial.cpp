#include "BeMaterial.h"

#include <cassert>
#include <sstream>
#include <iomanip>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"

auto BeMaterial::Create(
    std::string_view name,  const BeMaterialScheme& scheme, 
    bool frequentlyUsed,    const BeRenderer& renderer 
) -> std::shared_ptr<BeMaterial> 
{
    auto material = std::make_shared<BeMaterial>(std::string(name), frequentlyUsed, scheme, renderer);
    material->InitialiseSlotMaps();
    material->RebuildBindGroup();
    return material;
}

BeMaterial::BeMaterial(
    std::string name, const bool frequentlyUsed, BeMaterialScheme scheme, const BeRenderer& renderer
)   : Name(std::move(name)), _isFrequentlyUsed(frequentlyUsed), _scheme(std::move(scheme))
{
    static uint32_t materialCount = 0;
    _uniqueID = ++materialCount;

    if (!_scheme.Properties.empty()) {
        AssembleData();

        const uint32_t sizeInBytes = static_cast<uint32_t>(_bufferData.size() * sizeof(float));
        _cbuffer = SenBackend::CreateBuffer({
            .Usage  = SenBufferUsage::Constant,
            .Access = _isFrequentlyUsed ? SenBufferAccess::Dynamic : SenBufferAccess::Static,
            .Size   = sizeInBytes,
            .Data   = _bufferData.data(),
        });
    }
}

BeMaterial::~BeMaterial() {
    if (_bindGroup.IsValid()) {
        SenBackend::DestroyBindGroup(_bindGroup);
    }
}

auto BeMaterial::InitialiseSlotMaps() -> void {
    for (const auto& property : _scheme.Textures) {
        auto texWeak = BeAssetRegistry::GetTexture(property.DefaultTexturePath);
        be_assert(
            !texWeak.expired(), 
            "Texture not found in registry: " + property.DefaultTexturePath
        );

        _textures[property.Name] = {texWeak.lock(), property.SlotIndex};
    }

    for (const auto& property : _scheme.Samplers) {
        auto sampler = BeAssetRegistry::GetSampler(property.DefaultSamplerDescString);
        be_assert(sampler.IsValid(), "Invalid behaviour: BeAssetRegistry::GetSampler returned invalid handle. This should never happen");
        _samplers[property.Name] = { sampler, property.SlotIndex };
    }
}


auto BeMaterial::FlushBuffer() -> void {
    if (!_cbufferDirty) 
        return;
    
    SenBackend::WriteBuffer(_cbuffer, _bufferData.data(), static_cast<uint32_t>(_bufferData.size() * sizeof(float)));
    _cbufferDirty = false;
}

auto BeMaterial::GetBindGroupLayout() const -> SenBindGroupDesc {
    return _scheme.BindGroupLayout;
}

// rename to RetrieveBindGroup
auto BeMaterial::GetBindGroup() -> SenBindGroup {
    FlushBuffer();
    if (_bindGroupDirty) {
        RebuildBindGroup();
    }
    return _bindGroup;
}

auto BeMaterial::RebuildBindGroup() -> void {
    if (_bindGroup.IsValid()) {
        SenBackend::DestroyBindGroup(_bindGroup);
    }
    _bindGroup = SenBackend::CreateBindGroup(BuildBindGroupDesc());
    _bindGroupDirty = false;
}

auto BeMaterial::BuildBindGroupDesc() const -> SenBindGroupDesc {
    SenBindGroupDesc desc = _scheme.BindGroupLayout;

    desc.Textures.clear();
    desc.Textures.reserve(desc.TextureSlots.size());
    for (const auto textureSlot : desc.TextureSlots) {
        for (const auto& [_, pair] : _textures) {
            const auto& [texture, slot] = pair;
            if (slot == textureSlot && texture && texture->Handle.IsValid()) {
                desc.Textures.push_back(texture->Handle);
                break;
            }
        }
    }

    desc.Samplers.clear();
    desc.Samplers.reserve(desc.SamplerSlots.size());
    for (const auto samplerSlot : desc.SamplerSlots) {
        for (const auto& [_, pair] : _samplers) {
            const auto& [sampler, slot] = pair;
            if (slot == samplerSlot && sampler.IsValid()) {
                desc.Samplers.push_back(sampler);
                break;
            }
        }
    }

    desc.Buffers.clear();
    if (_cbuffer.IsValid()) {
        desc.Buffers.push_back(_cbuffer);
    }

    return desc;
}


auto BeMaterial::AssembleData() -> void {
    uint32_t offsetBytes = 0;

    for (const auto& property : _scheme.Properties) {
        constexpr uint32_t registerSizeBytes = 16;

        static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> SizeMap = {
            {BeMaterialPropertyDescriptor::Type::Float,  uint32_t(1 * sizeof(float))},
            {BeMaterialPropertyDescriptor::Type::Float2, uint32_t(2 * sizeof(float))},
            {BeMaterialPropertyDescriptor::Type::Float3, uint32_t(3 * sizeof(float))},
            {BeMaterialPropertyDescriptor::Type::Float4, uint32_t(4 * sizeof(float))},
            {BeMaterialPropertyDescriptor::Type::Matrix, uint32_t(16 * sizeof(float))},
        };
        
        const uint32_t elementSizeBytes = SizeMap.at(property.PropertyType);
        const uint32_t positionInRegister = offsetBytes % registerSizeBytes;
        
        if (positionInRegister + elementSizeBytes > registerSizeBytes && positionInRegister != 0) {
            offsetBytes = ((offsetBytes / registerSizeBytes) + 1) * registerSizeBytes;
        }

        _propertyOffsets[property.Name] = offsetBytes / sizeof(float);
        offsetBytes += elementSizeBytes;
    }

    _bufferData.resize(offsetBytes / 4);
    for (const auto& property : _scheme.Properties) {
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



auto BeMaterial::SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void {
    assert(_textures.contains(propertyName));
    _textures.at(propertyName).first = texture;
    _bindGroupDirty = true;
}

auto BeMaterial::GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture> {
    assert(_textures.contains(propertyName));
    return _textures.at(propertyName).first;
}



auto BeMaterial::SetSampler(const std::string& propertyName, SenSampler sampler) -> void {
    assert(_samplers.contains(propertyName));
    _samplers.at(propertyName).first = sampler;
    _bindGroupDirty = true;
}

auto BeMaterial::GetSampler(const std::string& propertyName) const -> SenSampler {
    assert(_samplers.contains(propertyName));
    return _samplers.at(propertyName).first;
}
