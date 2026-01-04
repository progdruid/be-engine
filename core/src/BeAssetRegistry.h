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
    static std::unordered_map<std::filesystem::path, std::string> _shaderSources;
    
    static std::unordered_map<std::string, std::shared_ptr<BeShader>> _shaders;
    static std::unordered_map<std::string, BeMaterialScheme> _materialSchemes;
    static std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    static std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textures;
    static std::unordered_map<std::string, std::shared_ptr<BeModel>> _models;

    expose
    //BeAssetRegistry() = default;
    //~BeAssetRegistry() = default;
    BeAssetRegistry() = delete;
    ~BeAssetRegistry() = delete;

    // Shaders
    static auto IndexShaderFiles (const std::vector<std::filesystem::path>& filePaths, const BeRenderer& renderer) -> void;
    
    static auto GetShader(std::string_view name) -> std::weak_ptr<BeShader> { assert(_shaders.contains(std::string(name))); return _shaders.at(std::string(name)); }
    static auto HasShader(std::string_view name) -> bool { return _shaders.contains(std::string(name)); }

    static auto GetMaterialScheme(std::string_view name) -> BeMaterialScheme { assert(_materialSchemes.contains(std::string(name))); return _materialSchemes.at(std::string(name)); }
    static auto HasMaterialScheme(std::string_view name) -> bool { return _materialSchemes.contains(std::string(name)); }
    
    // Material
    static auto AddMaterial(std::string_view name, std::shared_ptr<BeMaterial> material) -> void { _materials[std::string(name)] = material; }
    static auto GetMaterial(std::string_view name) -> std::weak_ptr<BeMaterial> { assert(_materials.contains(std::string(name))); return _materials.at(std::string(name)); }
    static auto RemoveMaterial(std::string_view name) -> void { _materials.erase(std::string(name)); }
    static auto HasMaterial(std::string_view name) -> bool { return _materials.contains(std::string(name)); }

    // Texture
    static auto AddTexture(std::string_view name, std::shared_ptr<BeTexture> resource) -> void { _textures[std::string(name)] = resource; }
    static auto GetTexture(std::string_view name) -> std::weak_ptr<BeTexture> { assert(_textures.contains(std::string(name))); return _textures.at(std::string(name)); }
    static auto RemoveTexture(std::string_view name) -> void { _textures.erase(std::string(name)); }
    static auto HasTexture(std::string_view name) -> bool { return _textures.contains(std::string(name)); }
    
    // Model
    static auto AddModel(std::string_view name, std::shared_ptr<BeModel> model) -> void { _models[std::string(name)] = model; }
    static auto GetModel(std::string_view name) -> std::weak_ptr<BeModel> { assert(_models.contains(std::string(name))); return _models.at(std::string(name)); }
    static auto RemoveModel(std::string_view name) -> void { _models.erase(std::string(name)); }
    static auto HasModel(std::string_view name) -> bool { return _models.contains(std::string(name)); }
};
