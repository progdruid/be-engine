#pragma once

#include <d3d11.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "Utils.h"

class BeShaderIncludeHandler;
class BeRenderer;
using Microsoft::WRL::ComPtr;
using Json = nlohmann::ordered_json;


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

struct BeMaterialPropertyDescriptor {
    enum class Type : uint8_t {
        Float,
        Float2,
        Float3,
        Float4,
        Matrix
    };

    std::string Name;
    Type PropertyType;
    std::vector<float> DefaultValue;
};

struct BeMaterialTextureDescriptor {
    std::string Name;
    uint8_t SlotIndex;
    std::string DefaultTexturePath;
};

struct BeMaterialSamplerDescriptor {
    std::string Name;
    uint8_t SlotIndex;
};

struct BeMaterialDescriptor {
    std::string TypeName;
    uint8_t SlotIndex;
    std::vector<BeMaterialPropertyDescriptor> Properties;
    std::vector<BeMaterialTextureDescriptor> Textures;
    std::vector<BeMaterialSamplerDescriptor> Samplers;
};

class BeShader {
    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static std::string StandardShaderIncludePath;
    expose static auto Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader>;
    
    hide static auto CompileBlob (
        const std::filesystem::path& filePath,
        const char* entrypointName,
        const char* target,
        BeShaderIncludeHandler* includeHandler
    ) -> ComPtr<ID3DBlob>;
    hide static auto ParseFor (const std::string& src, const std::string& target) -> Json;
    hide static auto Take (std::string_view str, size_t start, size_t end) -> std::string_view;
    hide static auto Trim (std::string_view str, const char* trimmedChars) -> std::string_view;
    hide static auto Split (std::string_view str, const char* delimiters) -> std::vector<std::string_view>;
    
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    expose std::unordered_map<std::string, BeMaterialDescriptor> MaterialDescriptors;

    
    // lifecycle ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose BeShader() = default;
    expose ~BeShader() = default;
};

