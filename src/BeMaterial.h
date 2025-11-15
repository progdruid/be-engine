#pragma once
#include <d3d11.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <wrl/client.h>

struct BeTexture;
class BeShader;

using Microsoft::WRL::ComPtr;

class BeMaterial {
public:
    std::string Name;
    BeShader* Shader;

private:
    std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>> _textures;
    std::unordered_map<std::string, uint32_t> _propertyOffsets;
    std::vector<float> _bufferData;
    ComPtr<ID3D11Buffer> _cbuffer = nullptr;
    bool _cbufferDirty;
    
public:
    explicit BeMaterial(std::string name, BeShader* shader, const ComPtr<ID3D11Device>& device);
    ~BeMaterial();

    BeMaterial(const BeMaterial& other) = default;
    BeMaterial(BeMaterial&& other) noexcept = default;
    BeMaterial& operator=(const BeMaterial& other) = default;
    BeMaterial& operator=(BeMaterial&& other) noexcept = default;

    auto SetFloat  (const std::string& propertyName, float value) -> void;
    auto SetFloat2 (const std::string& propertyName, glm::vec2 value) -> void;
    auto SetFloat3 (const std::string& propertyName, glm::vec3 value) -> void;
    auto SetFloat4 (const std::string& propertyName, glm::vec4 value) -> void;
    auto SetTexture(const std::string& propertyName, const std::shared_ptr<BeTexture>& texture) -> void;

    auto GetFloat  (const std::string& propertyName) const -> float;
    auto GetFloat2 (const std::string& propertyName) const -> glm::vec2;
    auto GetFloat3 (const std::string& propertyName) const -> glm::vec3;
    auto GetFloat4 (const std::string& propertyName) const -> glm::vec4;
    auto GetTexture(const std::string& propertyName) const -> std::shared_ptr<BeTexture>;

    auto UpdateGPUBuffers (const ComPtr<ID3D11DeviceContext>& context) -> void;
    auto GetBuffer () const -> const ComPtr<ID3D11Buffer>& { return _cbuffer; }
    auto GetTexturePairs () const -> const std::unordered_map<std::string, std::pair<std::shared_ptr<BeTexture>, uint8_t>>& { return _textures; }
    
    auto Print() const -> std::string;

private:
    auto CalculateLayout() -> void;
};
