#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <umbrellas/common.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeMaterialScheme.h"
#include "BeShaderTools.h"

enum class BeShaderType : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel = 1 << 1,
    Tesselation = 1 << 2,
    Compute = 1 << 3,
    AllGraphics = Vertex | Pixel | Tesselation,
};
ENABLE_BITMASK(BeShaderType);

struct BeShader {
    struct MaterialSchemeEntry {
        std::string Link;
        BeMaterialScheme Scheme;
        uint8_t Index;
    };

    std::string Name;
    uint32_t ShaderID = 0;
    BeShaderType ShaderType = BeShaderType::None;

    SenTopology Topology = SenTopology::Undefined;
    SenRasterizerState RasterizerState;
    SenBlendState BlendState;
    SenDepthStencilState DepthStencilState;
    std::vector<SenVertexLayoutElement> VertexLayout;
    uint32_t VertexStride = 0;

    SenShader ShaderVertex;
    SenShader ShaderHull;
    SenShader ShaderDomain;
    SenShader ShaderPixel;
    SenShader ShaderCompute;

    std::unordered_map<std::string, uint32_t> PixelTargets;
    std::unordered_map<uint32_t, std::string> PixelTargetsInverse;

    bool HasMaterial = false;
    std::vector<MaterialSchemeEntry> MaterialSchemes;
    BeShaderTools::RootLayout RootLayout;
};
