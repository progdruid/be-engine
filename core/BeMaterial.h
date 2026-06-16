#pragma once
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeMaterialScheme.h"
#include "sen-rhi/SenTypes.h"

class BeRenderer;
class BeTexture;

class BeMaterial {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Create(std::string_view schemeName, bool frequentlyUsed) -> std::shared_ptr<BeMaterial>;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide uint32_t _uniqueID;
    hide bool _isFrequentlyUsed;
    
    BeMaterialScheme _scheme;
    SenBindGroup _bindGroup;
    bool _bindGroupDirty;

    struct TextureBinding {
        std::shared_ptr<BeTexture> Texture;
        uint8_t Slot;
        bool IsStorage;
        uint32_t Mip = SEN_FULL_MIPS; // SEN_FULL_MIPS = full-mip SRV
    };
    std::unordered_map<std::string, TextureBinding> _textures;
    std::unordered_map<std::string, std::pair<SenSampler, uint8_t>> _samplers;
    std::unordered_map<std::string, uint32_t> _propertyOffsets;
    std::vector<float> _bufferData;
    SenBuffer _cbuffer;
    bool _cbufferDirty = false;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    ~BeMaterial();

    BeMaterial(const BeMaterial& other) = default;
    BeMaterial(BeMaterial&& other) noexcept = default;
    BeMaterial& operator=(const BeMaterial& other) = default;
    BeMaterial& operator=(BeMaterial&& other) noexcept = default;
    
    explicit BeMaterial(BeMaterialScheme scheme, const bool frequentlyUsed);

    hide
    auto InitialiseSlotMaps() -> void;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto GetSchemeName () const -> std::string { return _scheme.Name; }
    auto GetUniqueID () const -> uint32_t { return _uniqueID; }

    auto FlushBuffer () -> void;
    auto GetBindGroup () -> SenBindGroup;
    auto GetBindGroupLayout () const -> SenBindGroupDesc;

    auto Print() const -> std::string;
    
    auto SetFloat  (const std::string& propertyName, float value) -> void;
    auto SetFloat2 (const std::string& propertyName, glm::vec2 value) -> void;
    auto SetFloat3 (const std::string& propertyName, glm::vec3 value) -> void;
    auto SetFloat4 (const std::string& propertyName, glm::vec4 value) -> void;
    auto SetMatrix (const std::string& propertyName, glm::mat4x4 value) -> void;

    auto GetFloat  (const std::string& propertyName) const -> float;
    auto GetFloat2 (const std::string& propertyName) const -> glm::vec2;
    auto GetFloat3 (const std::string& propertyName) const -> glm::vec3;
    auto GetFloat4 (const std::string& propertyName) const -> glm::vec4;
    auto GetMatrix (const std::string& propertyName) const -> glm::mat4x4;
    
    auto SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture, uint32_t mip = SEN_FULL_MIPS) -> void;
    auto GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture>;
    auto GetTextures() const { return _textures | std::views::values; }

    auto SetSampler(const std::string& propertyName, SenSampler sampler) -> void;
    auto GetSampler(const std::string& propertyName) const -> SenSampler;
    
    // internal ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    auto AssembleData        () -> void;
    auto BuildBindGroupDesc  () const -> SenBindGroupDesc;
    auto RebuildBindGroup    () -> void;
};
