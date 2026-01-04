#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "BeMaterialScheme.h"

class BeRenderer;
class BeTexture;
class BeShader;
class BeMaterial;
struct BeModel;

class BeAssetRegistry {
    
    hide
    std::unordered_map<std::filesystem::path, std::string> _shaderSources;
    
    std::unordered_map<std::string, std::shared_ptr<BeShader>> _shaders;
    std::unordered_map<std::string, BeMaterialScheme> _materialSchemes;
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textures;
    std::unordered_map<std::string, std::shared_ptr<BeModel>> _models;

    expose
    //BeAssetRegistry() = default;
    //~BeAssetRegistry() = default;
    BeAssetRegistry() = delete;
    ~BeAssetRegistry() = delete;

    // Shaders
    auto IndexShaderFiles (const std::vector<std::filesystem::path>& filePaths, const BeRenderer& renderer) -> void;
    
    auto AddShader(std::string_view name, std::shared_ptr<BeShader> shader) -> void { _shaders[std::string(name)] = shader; }
    auto GetShader(std::string_view name) -> std::weak_ptr<BeShader> { assert(_shaders.contains(std::string(name))); return _shaders.at(std::string(name)); }
    auto HasShader(std::string_view name) -> bool { return _shaders.contains(std::string(name)); }

    auto GetMaterialScheme(std::string_view name) -> BeMaterialScheme { assert(_materialSchemes.contains(std::string(name))); return _materialSchemes.at(std::string(name)); }
    auto HasMaterialScheme(std::string_view name) -> bool { return _materialSchemes.contains(std::string(name)); }
    
    // Material
    auto AddMaterial(std::string_view name, std::shared_ptr<BeMaterial> material) -> void { _materials[std::string(name)] = material; }
    auto GetMaterial(std::string_view name) -> std::weak_ptr<BeMaterial> { assert(_materials.contains(std::string(name))); return _materials.at(std::string(name)); }
    auto RemoveMaterial(std::string_view name) -> void { _materials.erase(std::string(name)); }
    auto HasMaterial(std::string_view name) -> bool { return _materials.contains(std::string(name)); }

    // Texture
    auto AddTexture(std::string_view name, std::shared_ptr<BeTexture> resource) -> void { _textures[std::string(name)] = resource; }
    auto GetTexture(std::string_view name) -> std::weak_ptr<BeTexture> { assert(_textures.contains(std::string(name))); return _textures.at(std::string(name)); }
    auto RemoveTexture(std::string_view name) -> void { _textures.erase(std::string(name)); }
    auto HasTexture(std::string_view name) -> bool { return _textures.contains(std::string(name)); }
    
    // Model
    auto AddModel(std::string_view name, std::shared_ptr<BeModel> model) -> void { _models[std::string(name)] = model; }
    auto GetModel(std::string_view name) -> std::weak_ptr<BeModel> { assert(_models.contains(std::string(name))); return _models.at(std::string(name)); }
    auto RemoveModel(std::string_view name) -> void { _models.erase(std::string(name)); }
    auto HasModel(std::string_view name) -> bool { return _models.contains(std::string(name)); }
};
