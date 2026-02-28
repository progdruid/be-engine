#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>

#include "BeTypes.h"

class BeRenderer;
struct BeShaderImpl;

struct BeVertexElementDescriptor {
    enum class BeVertexSemantic : uint8_t {
        Position,
        Normal,
        Color3,
        Color4,
        TexCoord0,
        TexCoord1,
        TexCoord2,
        Count_
    };

    std::string Name;
    BeVertexSemantic Attribute;
};

class BeShader {
    expose static std::string StandardShaderIncludePath;
    expose static auto Create(const std::filesystem::path& filePath, BeRenderer& renderer) -> std::shared_ptr<BeShader>;

    expose std::string Name;
    expose BeShaderType ShaderType = BeShaderType::None;
    expose BeTopology Topology = BeTopology::Undefined;
    expose std::unordered_map<std::string, uint32_t> PixelTargets;
    expose std::unordered_map<uint32_t, std::string> PixelTargetsInverse;

    expose bool HasMaterial = false;
    hide std::unordered_map<std::string, std::string> _materialSchemeNames;
    hide std::unordered_map<std::string, uint8_t> _materialSlots;
    hide std::unordered_map<std::string, uint8_t> _materialSlotsByScheme;

    hide std::unique_ptr<BeShaderImpl> _impl;

    expose BeShader();
    expose ~BeShader();

    expose auto GetMaterialSchemeName(const std::string& linkName) const -> std::string {
        return _materialSchemeNames.at(linkName);
    }
    expose auto GetMaterialSlot(const std::string& linkName) const -> uint8_t {
        return _materialSlots.at(linkName);
    }
    expose auto GetMaterialSlotByScheme(const std::string& schemeName) const -> uint8_t {
        return _materialSlotsByScheme.at(schemeName);
    }

    expose auto GetPlatformImpl() const -> BeShaderImpl* { return _impl.get(); }
};
