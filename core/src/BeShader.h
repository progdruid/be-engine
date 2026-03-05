#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "Utils.h"

class BeShaderIncludeHandler;
class BeRenderer;

enum class BeShaderType : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel = 1 << 1,
    Tesselation = 1 << 2,
    All = Vertex | Pixel | Tesselation
};
ENABLE_BITMASK(BeShaderType);

class BeShader {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader>;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose BeShaderType ShaderType = BeShaderType::None;
    expose SenTopology Topology = SenTopology::Undefined;
    expose SenShader ShaderVertex;
    expose SenShader ShaderHull;
    expose SenShader ShaderDomain;
    expose SenShader ShaderPixel;
    expose std::unordered_map<std::string, uint32_t> PixelTargets;
    expose std::unordered_map<uint32_t, std::string> PixelTargetsInverse;

    expose SenPipelineDesc _pipelineDesc;

    expose bool HasMaterial = false;
    hide std::unordered_map<std::string, std::string> _materialSchemeNames;
    hide std::unordered_map<std::string, uint8_t> _materialSlots;
    hide std::unordered_map<std::string, uint8_t> _materialSlotsByScheme;

    // lifecycle ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeShader() = default;
    expose ~BeShader() = default;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto GetMaterialSchemeName (const std::string& linkName) const -> std::string {
        return _materialSchemeNames.at(linkName);
    }
    expose auto GetMaterialSlot (const std::string& linkName) const -> uint8_t {
        return _materialSlots.at(linkName);
    }
    expose auto GetMaterialSlotByScheme (const std::string& schemeName) const -> uint8_t {
        return _materialSlotsByScheme.at(schemeName);
    }

    expose auto CreatePipelineDesc() const -> SenPipelineDesc {
        return _pipelineDesc;
    }
};

