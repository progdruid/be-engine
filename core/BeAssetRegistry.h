#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "umbrellas/include-libassert.h"
#include <umbrellas/common.hpp>

#ifdef GetProp
#undef GetProp
#endif

class BeTexture;
class BeMaterial;
struct BeProp;

class BeAssetRegistry {

    hide
    std::unordered_map<std::string, std::shared_ptr<BeMaterial>> _materials;
    std::unordered_map<std::string, std::shared_ptr<BeTexture>> _textures;
    std::unordered_map<std::string, std::shared_ptr<BeProp>> _props;

    expose
    explicit BeAssetRegistry();
    ~BeAssetRegistry();

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
