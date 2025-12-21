#pragma once
#include <d3d11.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>
#include <wrl/client.h>

#include "BeShader.h"

class BeRenderer;
class BeTexture;
using Microsoft::WRL::ComPtr;

class BeShader;
class BeAssetRegistry;

class BeMaterial {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Create(
        std::string_view name,
        bool frequentlyUsed,
        std::weak_ptr<BeShader> shader,
        const BeRenderer& renderer
    ) -> std::shared_ptr<BeMaterial>;

    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    std::string Name;
    std::weak_ptr<BeShader> Shader;

    hide
    bool _isFrequentlyUsed;
    
    std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>> _textures;
    std::unordered_map<std::string, std::pair<ComPtr<ID3D11SamplerState>, uint8_t>> _samplers;

    std::unordered_map<std::string, uint32_t> _propertyOffsets;
    std::vector<float> _bufferData;
    ComPtr<ID3D11Buffer> _cbuffer = nullptr;
    bool _cbufferDirty;

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
        const std::weak_ptr<BeShader>& shader,
        const BeRenderer& renderer
    );

    hide
    auto InitialiseSlotMaps(const BeRenderer& renderer) -> void;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
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

    auto SetSampler(const std::string& propertyName, ComPtr<ID3D11SamplerState> sampler) -> void;
    auto GetSampler(const std::string& propertyName) const -> ComPtr<ID3D11SamplerState>;
    
    auto UpdateGPUBuffers (const ComPtr<ID3D11DeviceContext>& context) -> void;
    auto GetBuffer () const -> const ComPtr<ID3D11Buffer>& { return _cbuffer; }
    auto GetTexturePairs () const -> const std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>>& { return _textures; }
    auto GetSamplerPairs () const -> const std::unordered_map<std::string, std::pair<ComPtr<ID3D11SamplerState>, uint8_t>>& { return _samplers; }
    
    auto Print() const -> std::string;

    // internal ////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto AssembleData() -> void;
};
