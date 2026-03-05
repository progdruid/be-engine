#pragma once
#include <memory>
#include <string>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

#include "BeMaterialScheme.h"
#include "sen-rhi/SenTypes.h"

class BeRenderer;
class BeTexture;

class BeMaterial {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Create(
        std::string_view name,
        const BeMaterialScheme& scheme,
        bool frequentlyUsed,
        const BeRenderer& renderer
    ) -> std::shared_ptr<BeMaterial>;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    std::string Name;
    
    hide
    bool _isFrequentlyUsed;
    BeMaterialScheme _scheme;
    uint32_t _uniqueID;

    std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>> _textures;
    std::unordered_map<std::string, std::pair<SenSampler, uint8_t>> _samplers;

    std::unordered_map<std::string, uint32_t> _propertyOffsets;
    std::vector<float> _bufferData;
    SenBuffer _cbuffer;
    bool _cbufferDirty = false;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    //BeMaterial();
    ~BeMaterial();

    BeMaterial(const BeMaterial& other) = default;
    BeMaterial(BeMaterial&& other) noexcept = default;
    BeMaterial& operator=(const BeMaterial& other) = default;
    BeMaterial& operator=(BeMaterial&& other) noexcept = default;

    explicit BeMaterial(
        std::string name,
        const bool frequentlyUsed,
        BeMaterialScheme descriptor,
        const BeRenderer& renderer
    );

    hide
    auto InitialiseSlotMaps() -> void;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto GetSchemeName () const -> std::string { return _scheme.Name; }
    auto GetUniqueID () const -> uint32_t { return _uniqueID; }
    
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
    
    auto SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void;
    auto GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture>;

    auto SetSampler(const std::string& propertyName, SenSampler sampler) -> void;
    auto GetSampler(const std::string& propertyName) const -> SenSampler;
    
    auto UpdateGPUBuffers () -> bool;
    auto GetBuffer () const -> SenBuffer { return _cbuffer; }
    auto GetTexturePairs () const -> const std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>>& { return _textures; }
    auto GetSamplerPairs () const -> const std::unordered_map<std::string, std::pair<SenSampler, uint8_t>>& { return _samplers; }
    
    auto Print() const -> std::string;

    // internal ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto AssembleData() -> void;
};
