#include "BeMaterial.h"

#include <sstream>

#include "BeAssetRegistry.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"

// std140 array stride is 16 bytes / 4 floats (float->float4); 
// matrix os 64 bytes / 16 floats.
static constexpr uint32_t ArrayStrideFloats = 4;
static constexpr uint32_t MatrixStrideFloats = 16;

static constexpr auto ArrayStrideFloatsFor(BeMaterialPropertyDescriptor::Type type) -> uint32_t {
    return type == BeMaterialPropertyDescriptor::Type::Matrix ? MatrixStrideFloats : ArrayStrideFloats;
}

auto BeMaterial::Create(const BeMaterialScheme& scheme) -> std::shared_ptr<BeMaterial> {
    auto material = std::make_shared<BeMaterial>(scheme);
    material->InitialiseSlotMaps();
    return material;
}

BeMaterial::BeMaterial(BeMaterialScheme scheme) : _scheme(std::move(scheme)) {
    static uint32_t materialCount = 0;
    _uniqueID = ++materialCount;

    if (!_scheme.Properties.empty()) {
        AssembleData();
        _arena = BeMaterialArena::Acquire(_scheme);
        _dynamicOffsets.resize(1);
        _cbufferDirty = true;
    }
}

BeMaterial::~BeMaterial() {
    for (const auto& [_, group] : _bindGroups) {
        SenBackend::RetireBindGroup(group);
    }
}

auto BeMaterial::InitialiseSlotMaps() -> void {
    for (const auto& property : _scheme.Textures) {
        auto texWeak = BeShaderLibrary::GetDefaultTexture(property.DefaultTexturePath);
        be_assert(!texWeak.expired(), "Default texture not found: " + property.DefaultTexturePath);
        _textures[property.Name] = {texWeak.lock(), property.SlotIndex, property.IsStorage};
    }

    for (const auto& property : _scheme.Samplers) {
        auto sampler = BeShaderLibrary::GetSampler(property.DefaultSamplerDescString);
        be_assert(sampler.IsValid(), "Invalid behaviour: BeShaderLibrary::GetSampler returned invalid handle. This should never happen");
        _samplers[property.Name] = { sampler, property.SlotIndex };
    }
}


auto BeMaterial::GetBindGroupLayout() const -> SenBindGroupDesc {
    return _scheme.BindGroupLayout;
}

// rename to RetrieveBindGroup
auto BeMaterial::GetBindGroup() -> SenBindGroupBinding {
    for (auto& binding : _textures | std::views::values) {
        if (binding.Texture && binding.Texture->Generation != binding.CachedGeneration) {
            binding.CachedGeneration = binding.Texture->Generation;
            _bindGroupDirty = true;
        }
    }

    if (_bindGroupDirty) {
        for (const auto& group : _bindGroups | std::views::values) {
            SenBackend::RetireBindGroup(group);
        }
        _bindGroups.clear();
        _bindGroupDirty = false;
    }

    if (_arena == nullptr) {
        return AcquireBindGroup({});
    }

    CommitChunk();
    return { AcquireBindGroup(_arena->GetBuffer(_chunk)), _dynamicOffsets };
}

auto BeMaterial::CommitChunk() -> void {
    const uint64_t frame = BeRenderer::GetCurrentFrame();
    const bool rewound = !_chunk.IsValid() || frame - _chunk.Frame >= BeRenderer::FramesInFlight;

    if (!rewound && !_cbufferDirty) {
        return;
    }

    _chunk = _arena->Allocate(frame);
    _dynamicOffsets[0] = _arena->GetOffset(_chunk);
    SenBackend::WriteBuffer(
        _arena->GetBuffer(_chunk),
        _bufferData.data(),
        uint32_t(_bufferData.size() * sizeof(float)),
        _dynamicOffsets[0]
    );
    _cbufferDirty = false;
}

auto BeMaterial::AcquireBindGroup(SenBuffer cbuffer) -> SenBindGroup {
    const auto it = _bindGroups.find(cbuffer.ID);
    if (it != _bindGroups.end()) {
        return it->second;
    }
    const SenBindGroup group = SenBackend::CreateBindGroup(BuildBindGroupDesc(cbuffer));
    _bindGroups[cbuffer.ID] = group;
    return group;
}

auto BeMaterial::BuildBindGroupDesc(SenBuffer cbuffer) const -> SenBindGroupDesc {
    SenBindGroupDesc desc = _scheme.BindGroupLayout;

    desc.Textures.clear();
    desc.TextureMips.clear();
    desc.Textures.reserve(desc.TextureSlots.size());
    desc.TextureMips.reserve(desc.TextureSlots.size());
    for (const auto textureSlot : desc.TextureSlots) {
        for (const auto& [_, binding] : _textures) {
            if (!binding.IsStorage && binding.Slot == textureSlot && binding.Texture && binding.Texture->Handle.IsValid()) {
                desc.Textures.push_back(binding.Texture->Handle);
                desc.TextureMips.push_back(binding.Mip);
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
    if (cbuffer.IsValid()) {
        desc.Buffers.push_back(cbuffer);
    }

    return desc;
}


auto BeMaterial::AssembleData() -> void {
    static const std::unordered_map<BeMaterialPropertyDescriptor::Type, uint32_t> ComponentMap = {
        {BeMaterialPropertyDescriptor::Type::Float,  1},
        {BeMaterialPropertyDescriptor::Type::Float2, 2},
        {BeMaterialPropertyDescriptor::Type::Float3, 3},
        {BeMaterialPropertyDescriptor::Type::Float4, 4},
        {BeMaterialPropertyDescriptor::Type::Matrix, 16},
    };

    _bufferData.resize(_scheme.CbufferSize / sizeof(float));
    for (const auto& property : _scheme.Properties) {
        const uint32_t propertyOffset = _scheme.PropertyOffsets.at(property.Name);
        const auto& defaultValue = property.DefaultValue;
        if (property.ArrayLength > 1) {
            const uint32_t comp = ComponentMap.at(property.PropertyType);
            const uint32_t stride = ArrayStrideFloatsFor(property.PropertyType);
            for (uint32_t i = 0; i < property.ArrayLength; ++i) {
                memcpy(_bufferData.data() + propertyOffset + i * stride,
                       defaultValue.data() + i * comp, comp * sizeof(float));
            }
        } else {
            memcpy(_bufferData.data() + propertyOffset, defaultValue.data(), defaultValue.size() * sizeof(float));
        }
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


auto BeMaterial::SetFloat1(const std::string& propertyName, float value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(float));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat2(const std::string& propertyName, glm::vec2 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec2));
    _cbufferDirty = true;
}
    
auto BeMaterial::SetFloat3(const std::string& propertyName, glm::vec3 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec3));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat4(const std::string& propertyName, glm::vec4 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec4));
    _cbufferDirty = true;
}

auto BeMaterial::SetMatrix(const std::string& propertyName, glm::mat4x4 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    memcpy(_bufferData.data() + offset, glm::value_ptr(value), sizeof(glm::mat4x4));
    _cbufferDirty = true;
}

auto BeMaterial::GetFloat(const std::string& propertyName) const -> float {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    float value;
    memcpy(&value, _bufferData.data() + offset, sizeof(float));
    return value;
}

auto BeMaterial::GetFloat2(const std::string& propertyName) const -> glm::vec2 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    glm::vec2 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec2));
    return value;
}

auto BeMaterial::GetFloat3(const std::string& propertyName) const -> glm::vec3 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    glm::vec3 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec3));
    return value;
}

auto BeMaterial::GetFloat4(const std::string& propertyName) const -> glm::vec4 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    glm::vec4 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec4));
    return value;
}

auto BeMaterial::GetMatrix(const std::string& propertyName) const -> glm::mat4x4 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName);
    glm::mat4x4 value;
    memcpy(glm::value_ptr(value), _bufferData.data() + offset, sizeof(glm::mat4x4));
    return value;
}


auto BeMaterial::SetFloat1At(const std::string& propertyName, uint32_t index, float value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    memcpy(_bufferData.data() + offset, &value, sizeof(float));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat2At(const std::string& propertyName, uint32_t index, glm::vec2 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec2));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat3At(const std::string& propertyName, uint32_t index, glm::vec3 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec3));
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat4At(const std::string& propertyName, uint32_t index, glm::vec4 value) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    memcpy(_bufferData.data() + offset, &value, sizeof(glm::vec4));
    _cbufferDirty = true;
}


auto BeMaterial::SetFloat1Array(const std::string& propertyName, std::span<const float> values) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(values.size() <= _scheme.PropertyArrayLengths.at(propertyName), "too many array elements: " + propertyName);
    const uint32_t base = _scheme.PropertyOffsets.at(propertyName);
    for (size_t i = 0; i < values.size(); ++i) {
        memcpy(_bufferData.data() + base + i * ArrayStrideFloats, &values[i], sizeof(float));
    }
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat2Array(const std::string& propertyName, std::span<const glm::vec2> values) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(values.size() <= _scheme.PropertyArrayLengths.at(propertyName), "too many array elements: " + propertyName);
    const uint32_t base = _scheme.PropertyOffsets.at(propertyName);
    for (size_t i = 0; i < values.size(); ++i) {
        memcpy(_bufferData.data() + base + i * ArrayStrideFloats, &values[i], sizeof(glm::vec2));
    }
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat3Array(const std::string& propertyName, std::span<const glm::vec3> values) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(values.size() <= _scheme.PropertyArrayLengths.at(propertyName), "too many array elements: " + propertyName);
    const uint32_t base = _scheme.PropertyOffsets.at(propertyName);
    for (size_t i = 0; i < values.size(); ++i) {
        memcpy(_bufferData.data() + base + i * ArrayStrideFloats, &values[i], sizeof(glm::vec3));
    }
    _cbufferDirty = true;
}

auto BeMaterial::SetFloat4Array(const std::string& propertyName, std::span<const glm::vec4> values) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(values.size() <= _scheme.PropertyArrayLengths.at(propertyName), "too many array elements: " + propertyName);
    const uint32_t base = _scheme.PropertyOffsets.at(propertyName);
    for (size_t i = 0; i < values.size(); ++i) {
        memcpy(_bufferData.data() + base + i * ArrayStrideFloats, &values[i], sizeof(glm::vec4));
    }
    _cbufferDirty = true;
}

auto BeMaterial::SetMatrixArray(const std::string& propertyName, std::span<const glm::mat4x4> values) -> void {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(values.size() <= _scheme.PropertyArrayLengths.at(propertyName), "too many array elements: " + propertyName);
    const uint32_t base = _scheme.PropertyOffsets.at(propertyName);
    for (size_t i = 0; i < values.size(); ++i) {
        memcpy(_bufferData.data() + base + i * MatrixStrideFloats, glm::value_ptr(values[i]), sizeof(glm::mat4x4));
    }
    _cbufferDirty = true;
}


auto BeMaterial::GetFloat1At(const std::string& propertyName, uint32_t index) const -> float {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    float value;
    memcpy(&value, _bufferData.data() + offset, sizeof(float));
    return value;
}

auto BeMaterial::GetFloat2At(const std::string& propertyName, uint32_t index) const -> glm::vec2 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    glm::vec2 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec2));
    return value;
}

auto BeMaterial::GetFloat3At(const std::string& propertyName, uint32_t index) const -> glm::vec3 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    glm::vec3 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec3));
    return value;
}

auto BeMaterial::GetFloat4At(const std::string& propertyName, uint32_t index) const -> glm::vec4 {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    be_assert(index < _scheme.PropertyArrayLengths.at(propertyName), "array index out of range: " + propertyName);
    const uint32_t offset = _scheme.PropertyOffsets.at(propertyName) + index * ArrayStrideFloats;
    glm::vec4 value;
    memcpy(&value, _bufferData.data() + offset, sizeof(glm::vec4));
    return value;
}


auto BeMaterial::GetFloat1Array(const std::string& propertyName) const -> std::vector<float> {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t length = _scheme.PropertyArrayLengths.at(propertyName);
    std::vector<float> result(length);
    for (uint32_t i = 0; i < length; ++i) {
        result[i] = GetFloat1At(propertyName, i);
    }
    return result;
}

auto BeMaterial::GetFloat2Array(const std::string& propertyName) const -> std::vector<glm::vec2> {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t length = _scheme.PropertyArrayLengths.at(propertyName);
    std::vector<glm::vec2> result(length);
    for (uint32_t i = 0; i < length; ++i) {
        result[i] = GetFloat2At(propertyName, i);
    }
    return result;
}

auto BeMaterial::GetFloat3Array(const std::string& propertyName) const -> std::vector<glm::vec3> {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t length = _scheme.PropertyArrayLengths.at(propertyName);
    std::vector<glm::vec3> result(length);
    for (uint32_t i = 0; i < length; ++i) {
        result[i] = GetFloat3At(propertyName, i);
    }
    return result;
}

auto BeMaterial::GetFloat4Array(const std::string& propertyName) const -> std::vector<glm::vec4> {
    be_assert(_scheme.PropertyOffsets.contains(propertyName), "unknown material property: " + propertyName);
    const uint32_t length = _scheme.PropertyArrayLengths.at(propertyName);
    std::vector<glm::vec4> result(length);
    for (uint32_t i = 0; i < length; ++i) {
        result[i] = GetFloat4At(propertyName, i);
    }
    return result;
}



auto BeMaterial::SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture, uint32_t mip) -> void {
    be_assert(_textures.contains(propertyName), "unknown texture property: " + propertyName);
    auto& binding = _textures.at(propertyName);
    if (binding.Texture == texture && binding.Mip == mip) {
        return;
    }
    binding.Texture = texture;
    binding.Mip = mip;
    binding.CachedGeneration = texture ? texture->Generation : 0;
    _bindGroupDirty = true;
}

auto BeMaterial::GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture> {
    be_assert(_textures.contains(propertyName), "unknown texture property: " + propertyName);
    return _textures.at(propertyName).Texture;
}



auto BeMaterial::SetSampler(const std::string& propertyName, SenSampler sampler) -> void {
    be_assert(_samplers.contains(propertyName), "unknown sampler property: " + propertyName);
    auto& binding = _samplers.at(propertyName);
    if (binding.first.ID == sampler.ID) {
        return;
    }
    binding.first = sampler;
    _bindGroupDirty = true;
}

auto BeMaterial::GetSampler(const std::string& propertyName) const -> SenSampler {
    be_assert(_samplers.contains(propertyName), "unknown sampler property: " + propertyName);
    return _samplers.at(propertyName).first;
}
