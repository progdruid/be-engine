#include "BeMaterial.h"

#include <sstream>
#include <iomanip>

#include "BeAssetRegistry.h"
#include "BeRenderer.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"

auto BeMaterial::Create(std::string_view schemeName, bool frequentlyUsed) -> std::shared_ptr<BeMaterial> {
    auto scheme = BeAssetRegistry::GetMaterialScheme(schemeName);
    auto material = std::make_shared<BeMaterial>(scheme, frequentlyUsed);
    material->InitialiseSlotMaps();
    material->RebuildBindGroup();
    return material;
}

BeMaterial::BeMaterial(BeMaterialScheme scheme, const bool frequentlyUsed) : _isFrequentlyUsed(frequentlyUsed), _scheme(std::move(scheme)) {
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

        _textures[property.Name] = {texWeak.lock(), property.SlotIndex, property.IsStorage};
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
        for (const auto& [_, binding] : _textures) {
            if (!binding.IsStorage && binding.Slot == textureSlot && binding.Texture && binding.Texture->Handle.IsValid()) {
                desc.Textures.push_back(binding.Texture->Handle);
                break;
            }
        }
    }

    desc.StorageTextures.clear();
    desc.StorageTextures.reserve(desc.StorageTextureSlots.size());
    for (const auto storageSlot : desc.StorageTextureSlots) {
        for (const auto& [_, binding] : _textures) {
            if (binding.IsStorage && binding.Slot == storageSlot && binding.Texture && binding.Texture->Handle.IsValid()) {
                desc.StorageTextures.push_back(binding.Texture->Handle);
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

    // std140 layout — matches Vulkan UBOs, WebGPU/WGSL, and Slang's HLSL output.
    static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> SizeMap = {
        {BeMaterialPropertyDescriptor::Type::Float,  uint32_t(1 * sizeof(float))},
        {BeMaterialPropertyDescriptor::Type::Float2, uint32_t(2 * sizeof(float))},
        {BeMaterialPropertyDescriptor::Type::Float3, uint32_t(3 * sizeof(float))},
        {BeMaterialPropertyDescriptor::Type::Float4, uint32_t(4 * sizeof(float))},
        {BeMaterialPropertyDescriptor::Type::Matrix, uint32_t(16 * sizeof(float))},
    };
    static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> AlignMap = {
        {BeMaterialPropertyDescriptor::Type::Float,  4},
        {BeMaterialPropertyDescriptor::Type::Float2, 8},
        {BeMaterialPropertyDescriptor::Type::Float3, 16},
        {BeMaterialPropertyDescriptor::Type::Float4, 16},
        {BeMaterialPropertyDescriptor::Type::Matrix, 16},
    };

    for (const auto& property : _scheme.Properties) {
        const uint32_t size  = SizeMap.at(property.PropertyType);
        const uint32_t align = AlignMap.at(property.PropertyType);

        offsetBytes = (offsetBytes + align - 1) / align * align;

        _propertyOffsets[property.Name] = offsetBytes / sizeof(float);
        offsetBytes += size;
    }

    // Pad to next 16-byte boundary (std140 layout requirement)
    const uint32_t paddedBytes = ((offsetBytes + 15) / 16) * 16;
    _bufferData.resize(paddedBytes / 4);
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
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(float));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat2(const std::string& propertyName, glm::vec2 value) -> void {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec2));
    _cbufferDirty = true;
}
    
auto BeMaterial::SetFloat3(const std::string& propertyName, glm::vec3 value) -> void {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec3));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat4(const std::string& propertyName, glm::vec4 value) -> void {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec4));
    _cbufferDirty = true;
}

auto BeMaterial::SetMatrix(const std::string& propertyName, glm::mat4x4 value) -> void {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, glm::value_ptr(value), sizeof(glm::mat4x4));
    _cbufferDirty = true;
}

auto BeMaterial::GetFloat(const std::string& propertyName) const -> float {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    float value;
    memcpy(&value, _bufferData.data() + offset, sizeof(float));
    return value;
}

auto BeMaterial::GetFloat2(const std::string& propertyName) const -> glm::vec2 {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec2 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec2));
    return value;
}

auto BeMaterial::GetFloat3(const std::string& propertyName) const -> glm::vec3 {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec3 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec3));
    return value;
}

auto BeMaterial::GetFloat4(const std::string& propertyName) const -> glm::vec4 {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::vec4 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec4));
    return value;
}

auto BeMaterial::GetMatrix(const std::string& propertyName) const -> glm::mat4x4 {
    be_assert(_propertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _propertyOffsets.at(propertyName);
    glm::mat4x4 value;
    memcpy(glm::value_ptr(value), _bufferData.data() + offset, sizeof(glm::mat4x4));
    return value;
}



auto BeMaterial::SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void {
    be_assert(_textures.contains(propertyName), "unknown texture property: " + propertyName);
    _textures.at(propertyName).Texture = texture;
    _bindGroupDirty = true;
}

auto BeMaterial::GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture> {
    be_assert(_textures.contains(propertyName), "unknown texture property: " + propertyName);
    return _textures.at(propertyName).Texture;
}



auto BeMaterial::SetSampler(const std::string& propertyName, SenSampler sampler) -> void {
    be_assert(_samplers.contains(propertyName), "unknown sampler property: " + propertyName);
    _samplers.at(propertyName).first = sampler;
    _bindGroupDirty = true;
}

auto BeMaterial::GetSampler(const std::string& propertyName) const -> SenSampler {
    be_assert(_samplers.contains(propertyName), "unknown sampler property: " + propertyName);
    return _samplers.at(propertyName).first;
}
