#pragma once

#include <d3d11.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <filesystem>

#include "BeShaderIncludeHandler.hpp"
#include "Utils.h"
using Microsoft::WRL::ComPtr;



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

class BeShader {
public:
    //get
    const std::vector<BeVertexElementDescriptor> VertexLayout;
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> ComputedInputLayout;

    BeShaderType ShaderType = BeShaderType::None;

public:
    BeShader(
        ID3D11Device* device, 
        const std::filesystem::path& filePathWithoutExtension,
        const BeShaderType shaderType,
        const std::vector<BeVertexElementDescriptor>& vertexLayout);
    ~BeShader() = default;
    
    auto Bind (ID3D11DeviceContext* context) const -> void;

private:
    auto LoadVertexShader (const std::filesystem::path& filePath, const std::vector<BeVertexElementDescriptor>& vertexLayout, ID3D11Device* device, BeShaderIncludeHandler* includeHandler) -> void;
    auto LoadPixelShader (const std::filesystem::path& filePath, ID3D11Device* device, BeShaderIncludeHandler* includeHandler) -> void;
};

