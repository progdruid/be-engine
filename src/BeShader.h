#pragma once

#include <d3d11.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "BeShaderIncludeHandler.hpp"
#include "Utils.h"

using Microsoft::WRL::ComPtr;
using Json = nlohmann::ordered_json;


enum class BeShaderType : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel = 1 << 1,
};
ENABLE_BITMASK(BeShaderType);

struct BeVertexElementDescriptor {
    enum class BeVertexSemantic : uint8_t {
        Position,
        Normal,
        Color3,
        Color4,
        TexCoord0,
        TexCoord1,
        TexCoord2,
        _Count
    };

    static inline const std::unordered_map<std::string, BeVertexSemantic> SemanticMap = {
        {"position", BeVertexSemantic::Position},
        {"normal", BeVertexSemantic::Normal},
        {"color3", BeVertexSemantic::Color3},
        {"color4", BeVertexSemantic::Color4},
        {"uv0", BeVertexSemantic::TexCoord0},
        {"uv1", BeVertexSemantic::TexCoord1},
        {"uv2", BeVertexSemantic::TexCoord2},
    };
    static inline const std::unordered_map<BeVertexSemantic, const char*> SemanticNames = {
        {BeVertexSemantic::Position, "POSITION"},
        {BeVertexSemantic::Normal, "NORMAL"},
        {BeVertexSemantic::Color3, "COLOR"},
        {BeVertexSemantic::Color4, "COLOR"},
        {BeVertexSemantic::TexCoord0, "TEXCOORD"}, //????????
        {BeVertexSemantic::TexCoord1, "TEXCOORD1"},
        {BeVertexSemantic::TexCoord2, "TEXCOORD2"},
    };

    static inline const std::unordered_map<BeVertexSemantic, DXGI_FORMAT> ElementFormats = {
        {BeVertexSemantic::Position, DXGI_FORMAT_R32G32B32_FLOAT},
        {BeVertexSemantic::Normal, DXGI_FORMAT_R32G32B32_FLOAT},
        {BeVertexSemantic::Color3, DXGI_FORMAT_R32G32B32_FLOAT},
        {BeVertexSemantic::Color4, DXGI_FORMAT_R32G32B32A32_FLOAT},
        {BeVertexSemantic::TexCoord0, DXGI_FORMAT_R32G32_FLOAT},
        {BeVertexSemantic::TexCoord1, DXGI_FORMAT_R32G32_FLOAT},
        {BeVertexSemantic::TexCoord2, DXGI_FORMAT_R32G32_FLOAT},
    };
    static inline const std::unordered_map<BeVertexSemantic, uint32_t> SemanticSizes = {
        {BeVertexSemantic::Position, 12},
        {BeVertexSemantic::Normal, 12},
        {BeVertexSemantic::Color3, 12},
        {BeVertexSemantic::Color4, 16},
        {BeVertexSemantic::TexCoord0, 8},
        {BeVertexSemantic::TexCoord1, 8},
        {BeVertexSemantic::TexCoord2, 8},
    };
    static inline const std::unordered_map<BeVertexSemantic, uint32_t> ElementOffsets = {
        {BeVertexSemantic::Position,   0},
        {BeVertexSemantic::Normal,    12},
        {BeVertexSemantic::Color3,    24},
        {BeVertexSemantic::Color4,    24},
        {BeVertexSemantic::TexCoord0, 40},
        {BeVertexSemantic::TexCoord1, 48},
        {BeVertexSemantic::TexCoord2, 56},
    };
    
    std::string Name;
    BeVertexSemantic Attribute;
};

struct BeMaterialPropertyDescriptor {
    enum class Type : uint8_t {
        Float,
        Float2,
        Float3,
        Float4
    };

    static inline const std::unordered_map<Type, uint32_t> SizeMap = {
        {Type::Float,  1 * sizeof(float)},
        {Type::Float2, 2 * sizeof(float)},
        {Type::Float3, 3 * sizeof(float)},
        {Type::Float4, 4 * sizeof(float)},
    };

    std::string Name;
    Type PropertyType;
    std::vector<float> DefaultValue;
};

struct BeMaterialTexturePropertyDescriptor {
    std::string Name;
    uint8_t SlotIndex;
    std::string DefaultTexturePath;
};

class BeShader {
public:
    // Static
    static auto Create(ID3D11Device* device, const std::filesystem::path& filePath) -> std::shared_ptr<BeShader>;

    // Data
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> ComputedInputLayout;
    bool HasMaterial = false;
    std::vector<BeMaterialPropertyDescriptor> MaterialProperties;
    std::vector<BeMaterialTexturePropertyDescriptor> MaterialTextureProperties;


    // Lifecycle
    BeShader() = default;
    ~BeShader() = default;

    // Public methods
    auto Bind (ID3D11DeviceContext* context) const -> void;
    inline auto GetShaderType () const -> BeShaderType  { return _shaderType; }

private:
    BeShaderType _shaderType = BeShaderType::None;

    auto CompileBlob (
        const std::filesystem::path& filePath,
        const char* entrypointName,
        const char* target,
        BeShaderIncludeHandler* includeHandler
    ) -> ComPtr<ID3DBlob>;

    static auto ParseHeader (const std::string& src) -> Json;
    static auto Take (std::string_view str, size_t start, size_t end) -> std::string_view;
    static auto Trim (std::string_view str, const char* trimmedChars) -> std::string_view;
    static auto Split (std::string_view str, const char* delimiters) -> std::vector<std::string_view>;
};

