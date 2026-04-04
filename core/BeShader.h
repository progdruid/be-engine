#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include <umbrellas/bitmask.hpp>

#include "umbrellas/include-libassert.h"

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
    hide static uint32_t _shaderCount;
    expose static auto Create(const std::filesystem::path& filePath) -> std::shared_ptr<BeShader>;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose uint32_t ShaderID;
    expose BeShaderType ShaderType = BeShaderType::None;
    expose SenTopology Topology = SenTopology::Undefined;
    expose SenShader ShaderVertex;
    expose SenShader ShaderHull;
    expose SenShader ShaderDomain;
    expose SenShader ShaderPixel;
    expose std::unordered_map<std::string, uint32_t> PixelTargets;
    expose std::unordered_map<uint32_t, std::string> PixelTargetsInverse;
    
    hide SenPipelineDesc _pipelineDesc;

    expose bool HasMaterial = false;
    expose struct MaterialSchemeEntry {
        expose std::string Link;
        expose std::string SchemeName;
        expose uint8_t Index;
    };
    hide std::vector<MaterialSchemeEntry> _materialSchemes;

    // lifecycle ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeShader() = default;
    expose ~BeShader() = default;

    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto GetMaterialSchemeName (const std::string& linkName) const -> std::string {
        for (const auto & scheme : _materialSchemes) {
            if (scheme.Link == linkName) {
                return scheme.SchemeName;
            }
        }
        be_assert(false, "BeShader: didnt find material schemes under such link name.");
        return {};
    }
    expose auto GetMaterialSlot (const std::string& linkName) const -> uint8_t {
        for (const auto & scheme : _materialSchemes) {
            if (scheme.Link == linkName) {
                return scheme.Index;
            }
        }
        be_assert(false, "BeShader: didnt find material schemes under such link name.");
        return {};
    }
    expose auto GetMaterialSlotByScheme (const std::string& schemeName) const -> uint8_t {
        for (const auto & scheme : _materialSchemes) {
            if (scheme.SchemeName == schemeName) {
                return scheme.Index;
            }
        }
        be_assert(false, "BeShader: didnt find material schemes under such link name.");
        return {};
    } // all this bullshit will be deleted

    expose auto GetPipelineDesc() const -> SenPipelineDesc {
        return _pipelineDesc;
    }
};

