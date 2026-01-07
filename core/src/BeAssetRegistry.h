#pragma once

#include <cassert>
#include <d3d11.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <wrl/client.h>

#include "BeMaterialScheme.h"
#include "umbrellas/include-libassert.h"

class BeRenderer;
class BeTexture;
class BeShader;
class BeMaterial;
struct BeModel;

using Microsoft::WRL::ComPtr;

class BeAssetRegistry {
    
    hide
    static std::weak_ptr<BeRenderer> _renderer;
    
    static std::unordered_map<std::filesystem::path, std::string> _shaderSources;
    
    static std::unordered_map<std::string, std::shared_ptr<BeShader>> _shaders;
    static std::unordered_map<std::string, BeMaterialScheme> _materialSchemes;
    static std::unordered_map<std::string, ComPtr<ID3D11SamplerState>> _samplers;
    static std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    static std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textures;
    static std::unordered_map<std::string, std::shared_ptr<BeModel>> _models;

    expose
    //BeAssetRegistry() = default;
    //~BeAssetRegistry() = default;
    BeAssetRegistry() = delete;
    ~BeAssetRegistry() = delete;

    static auto InjectRenderer (const std::weak_ptr<BeRenderer>& renderer) -> void { _renderer = renderer; }
    
    // Shaders
    static auto IndexShaderFiles (const std::vector<std::filesystem::path>& filePaths) -> void;
    
    static auto GetShader(std::string_view name) -> std::weak_ptr<BeShader> {
        be_assert(_shaders.contains(std::string(name))); 
        return _shaders.at(std::string(name));
    }
    static auto HasShader(std::string_view name) -> bool { return _shaders.contains(std::string(name)); }

    static auto GetMaterialScheme(std::string_view name) -> BeMaterialScheme { be_assert(_materialSchemes.contains(std::string(name))); return _materialSchemes.at(std::string(name)); }
    static auto HasMaterialScheme(std::string_view name) -> bool { return _materialSchemes.contains(std::string(name)); }
    
    static auto GetSampler (std::string_view samplerDescString) -> ComPtr<ID3D11SamplerState>;
    
    // Material
    static auto AddMaterial(std::string_view name, std::shared_ptr<BeMaterial> material) -> void { _materials[std::string(name)] = material; }
    static auto GetMaterial(std::string_view name) -> std::weak_ptr<BeMaterial> { be_assert(_materials.contains(std::string(name))); return _materials.at(std::string(name)); }
    static auto RemoveMaterial(std::string_view name) -> void { _materials.erase(std::string(name)); }
    static auto HasMaterial(std::string_view name) -> bool { return _materials.contains(std::string(name)); }

    // Texture
    static auto AddTexture(std::string_view name, std::shared_ptr<BeTexture> resource) -> void { _textures[std::string(name)] = resource; }
    static auto GetTexture(std::string_view name) -> std::weak_ptr<BeTexture> { be_assert(_textures.contains(std::string(name))); return _textures.at(std::string(name)); }
    static auto RemoveTexture(std::string_view name) -> void { _textures.erase(std::string(name)); }
    static auto HasTexture(std::string_view name) -> bool { return _textures.contains(std::string(name)); }
    
    // Model
    static auto AddModel(std::string_view name, std::shared_ptr<BeModel> model) -> void { _models[std::string(name)] = model; }
    static auto GetModel(std::string_view name) -> std::weak_ptr<BeModel> { be_assert(_models.contains(std::string(name))); return _models.at(std::string(name)); }
    static auto RemoveModel(std::string_view name) -> void { _models.erase(std::string(name)); }
    static auto HasModel(std::string_view name) -> bool { return _models.contains(std::string(name)); }
};
