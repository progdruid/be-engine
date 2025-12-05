#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

class BeRenderResource;
class BeShader;
class BeMaterial;
struct BeModel;

class BeAssetRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<BeShader>> _shaders;
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    std::unordered_map<std::string, std::shared_ptr<BeRenderResource>> _resources;
    std::unordered_map<std::string, std::shared_ptr<BeModel>> _models;

public:
    BeAssetRegistry() = default;
    ~BeAssetRegistry() = default;

    // Shader
    auto AddShader(std::string_view name, std::shared_ptr<BeShader> shader) -> void { _shaders[std::string(name)] = shader; }
    auto GetShader(std::string_view name) -> std::weak_ptr<BeShader> { assert(_shaders.contains(std::string(name))); return _shaders.at(std::string(name)); }
    auto RemoveShader(std::string_view name) -> void { _shaders.erase(std::string(name)); }
    auto HasShader(std::string_view name) const -> bool { return _shaders.contains(std::string(name)); }

    // Material
    auto AddMaterial(std::string_view name, std::shared_ptr<BeMaterial> material) -> void { _materials[std::string(name)] = material; }
    auto GetMaterial(std::string_view name) -> std::weak_ptr<BeMaterial> { assert(_materials.contains(std::string(name))); return _materials.at(std::string(name)); }
    auto RemoveMaterial(std::string_view name) -> void { _materials.erase(std::string(name)); }
    auto HasMaterial(std::string_view name) const -> bool { return _materials.contains(std::string(name)); }

    // Texture
    auto AddResource(std::string_view name, std::shared_ptr<BeRenderResource> resource) -> void { _resources[std::string(name)] = resource; }
    auto GetResource(std::string_view name) -> std::weak_ptr<BeRenderResource> { assert(_resources.contains(std::string(name))); return _resources.at(std::string(name)); }
    auto RemoveResource(std::string_view name) -> void { _resources.erase(std::string(name)); }
    auto HasResource(std::string_view name) const -> bool { return _resources.contains(std::string(name)); }
    
    // Model
    auto AddModel(std::string_view name, std::shared_ptr<BeModel> model) -> void { _models[std::string(name)] = model; }
    auto GetModel(std::string_view name) -> std::weak_ptr<BeModel> { assert(_models.contains(std::string(name))); return _models.at(std::string(name)); }
    auto RemoveModel(std::string_view name) -> void { _models.erase(std::string(name)); }
    auto HasModel(std::string_view name) const -> bool { return _models.contains(std::string(name)); }
};
