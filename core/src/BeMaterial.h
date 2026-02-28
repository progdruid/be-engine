#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "BeTypes.h"
#include "BeMaterialScheme.h"

class BeRenderer;
class BeTexture;
struct BeMaterialImpl;

class BeMaterial {
    expose
    static auto Create(
        std::string_view name,
        const BeMaterialScheme& scheme,
        bool frequentlyUsed,
        BeRenderer& renderer
    ) -> std::shared_ptr<BeMaterial>;

    expose std::string Name;

    hide
    bool _isFrequentlyUsed;
    BeMaterialScheme _scheme;
    uint32_t _uniqueID;

    std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>> _textures;
    std::unordered_map<std::string, std::pair<BeSampler, uint8_t>> _samplers;

    std::unordered_map<std::string, uint32_t> _propertyOffsets;
    std::vector<float> _bufferData;
    std::unique_ptr<BeMaterialImpl> _platformImpl;
    bool _cbufferDirty;

    expose ~BeMaterial();

    BeMaterial(const BeMaterial& other) = delete;
    BeMaterial(BeMaterial&& other) noexcept;
    BeMaterial& operator=(const BeMaterial& other) = delete;
    BeMaterial& operator=(BeMaterial&& other) noexcept;

    explicit BeMaterial(
        std::string name,
        bool frequentlyUsed,
        BeMaterialScheme descriptor,
        BeRenderer& renderer
    );

    hide auto InitialiseSlotMaps() -> void;

    expose
    auto GetSchemeName() const -> std::string { return _scheme.Name; }
    auto GetUniqueID() const -> uint32_t { return _uniqueID; }

    auto SetFloat(const std::string& propertyName, float value) -> void;
    auto SetFloat2(const std::string& propertyName, glm::vec2 value) -> void;
    auto SetFloat3(const std::string& propertyName, glm::vec3 value) -> void;
    auto SetFloat4(const std::string& propertyName, glm::vec4 value) -> void;
    auto SetMatrix(const std::string& propertyName, glm::mat4x4 value) -> void;

    auto GetFloat(const std::string& propertyName) const -> float;
    auto GetFloat2(const std::string& propertyName) const -> glm::vec2;
    auto GetFloat3(const std::string& propertyName) const -> glm::vec3;
    auto GetFloat4(const std::string& propertyName) const -> glm::vec4;
    auto GetMatrix(const std::string& propertyName) const -> glm::mat4x4;

    auto SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void;
    auto GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture>;

    auto SetSampler(const std::string& propertyName, const BeSampler& sampler) -> void;
    auto GetSampler(const std::string& propertyName) const -> BeSampler;

    auto GetTexturePairs() const -> const std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>>& { return _textures; }
    auto GetSamplerPairsInternal() const -> const std::unordered_map<std::string, std::pair<BeSampler, uint8_t>>& { return _samplers; }

    auto Print() const -> std::string;

    expose auto GetPlatformImpl() const -> BeMaterialImpl* { return _platformImpl.get(); }

    hide auto AssembleData() -> void;
    hide auto CreatePlatformBuffer(BeRenderer& renderer) -> void;
    hide auto UpdatePlatformBuffer() -> bool;
};
