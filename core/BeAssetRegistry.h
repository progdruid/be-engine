#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "BeMaterialScheme.h"
#include "umbrellas/include-libassert.h"
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#ifdef GetProp
#undef GetProp
#endif

class BeTexture;
class BeShader;
class BeMaterial;
struct BeProp;

class BeAssetRegistry {

    hide
    static std::unordered_map<std::string, std::shared_ptr<BeTexture>> _defaultTextures;
    static std::unordered_map<std::string, SenSampler> _samplers;

    expose
    static auto RegisterDefaultTexture(std::string_view name, std::shared_ptr<BeTexture> texture) -> void;
    static auto GetDefaultTexture(std::string_view name) -> std::weak_ptr<BeTexture> { be_assert(_defaultTextures.contains(std::string(name))); return _defaultTextures.at(std::string(name)); }
    static auto HasDefaultTexture(std::string_view name) -> bool { return _defaultTextures.contains(std::string(name)); }
    static auto GetSampler(std::string_view samplerDescString) -> SenSampler;
    static auto StaticShutdown() -> void;

    hide
    std::unordered_map<std::filesystem::path, std::string> _shaderSources;
    std::unordered_map<std::string, std::unique_ptr<BeShader>> _shaders;
    std::unordered_map<std::string, BeMaterialScheme> _materialSchemes;
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textures;
    std::unordered_map<std::string, std::shared_ptr<BeProp>> _props;

    expose
    explicit BeAssetRegistry();
    ~BeAssetRegistry();

    expose // Shaders
    auto IndexShaderFiles(const std::vector<std::filesystem::path>& filePaths) -> void;

    auto GetShader(std::string_view name) -> raw_ptr<BeShader> { be_assert(_shaders.contains(std::string(name)), name); return _shaders.at(std::string(name)).get(); }
    auto HasShader(std::string_view name) const -> bool { return _shaders.contains(std::string(name)); }

    auto GetMaterialScheme(std::string_view name) -> BeMaterialScheme { be_assert(_materialSchemes.contains(std::string(name))); return _materialSchemes.at(std::string(name)); }
    auto HasMaterialScheme(std::string_view name) const -> bool { return _materialSchemes.contains(std::string(name)); }

    expose // Material
    auto AddMaterial(std::string_view name, std::shared_ptr<BeMaterial> material) -> void { _materials[std::string(name)] = material; }
    auto GetMaterial(std::string_view name) -> std::weak_ptr<BeMaterial> { be_assert(_materials.contains(std::string(name))); return _materials.at(std::string(name)); }
    auto RemoveMaterial(std::string_view name) -> void { _materials.erase(std::string(name)); }
    auto HasMaterial(std::string_view name) const -> bool { return _materials.contains(std::string(name)); }

    expose // Texture
    auto AddTexture(std::string_view name, std::shared_ptr<BeTexture> resource) -> void { _textures[std::string(name)] = resource; }
    auto GetTexture(std::string_view name) -> std::weak_ptr<BeTexture> { be_assert(_textures.contains(std::string(name))); return _textures.at(std::string(name)); }
    auto RemoveTexture(std::string_view name) -> void { _textures.erase(std::string(name)); }
    auto HasTexture(std::string_view name) const -> bool { return _textures.contains(std::string(name)); }

    expose // Prop
    auto AddProp(std::string_view name, std::shared_ptr<BeProp> prop) -> void { _props[std::string(name)] = prop; }
    auto GetProp(std::string_view name) -> std::weak_ptr<BeProp> { be_assert(_props.contains(std::string(name))); return _props.at(std::string(name)); }
    auto RemoveProp(std::string_view name) -> void { _props.erase(std::string(name)); }
    auto HasProp(std::string_view name) const -> bool { return _props.contains(std::string(name)); }
};
