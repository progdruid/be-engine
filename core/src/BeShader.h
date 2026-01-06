#pragma once

#include <d3d11.h>
#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "Utils.h"

class BeShaderIncludeHandler;
class BeRenderer;
using Microsoft::WRL::ComPtr;

enum class BeShaderType : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel = 1 << 1,
    Tesselation = 1 << 2,
    All = Vertex | Pixel | Tesselation
};
ENABLE_BITMASK(BeShaderType);

enum class BeShaderTopology : uint8_t {
    None,
    TriangleList,
    PatchList3,
};

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
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static std::string StandardShaderIncludePath;
    expose static auto Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader>;
    
    /// @brief: What a hairy function, compiles shader source code
    /// @return std::expected. 
    /// @return On success: shader blob as ComPtr<ID3DBlob>. 
    /// @return On error: std::pair with HRESULT and error blob as ComPtr<ID3DBlob>.
    hide static auto CompileBlob (
        const std::string& src,
        const char* entrypointName,
        const char* target,
        BeShaderIncludeHandler* includeHandler
    ) -> std::expected<
        ComPtr<ID3DBlob>, 
        std::pair <HRESULT, ComPtr<ID3DBlob>>
    >;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose BeShaderType ShaderType = BeShaderType::None;
    expose D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    expose ComPtr<ID3D11InputLayout> ComputedInputLayout;
    expose ComPtr<ID3D11VertexShader> VertexShader;
    expose ComPtr<ID3D11HullShader> HullShader;
    expose ComPtr<ID3D11DomainShader> DomainShader;
    expose ComPtr<ID3D11PixelShader> PixelShader;
    expose std::unordered_map<std::string, uint32_t> PixelTargets;
    expose std::unordered_map<uint32_t, std::string> PixelTargetsInverse;
    
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
};

