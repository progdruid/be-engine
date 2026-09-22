#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <common.hpp>
#include <SenTypes.h>

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

struct BeShaderStageCode {
    std::string FunctionName;
    std::vector<uint32_t> Bytecode;

    auto IsValid() const -> bool { return !Bytecode.empty(); }
    auto GetCode() const -> SenShaderCode { return { Bytecode.data(), uint32_t(Bytecode.size()) }; }
};

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
    SenFill Fill = SenFill::Solid;
    bool DepthClipEnable = true;
    SenBlendState BlendState;

    // command state, not baked into the pipeline
    SenCull Cull = SenCull::Back;
    SenFrontFace FrontFace = SenFrontFace::Clockwise;
    SenDepthState DepthState;
    std::vector<SenVertexLayoutElement> VertexLayout;
    uint32_t VertexStride = 0;

    std::filesystem::path SourcePath;
    std::vector<std::filesystem::path> Includes;

    BeShaderStageCode StageVertex;
    BeShaderStageCode StageHull;
    BeShaderStageCode StageDomain;
    BeShaderStageCode StagePixel;
    BeShaderStageCode StageCompute;

    std::unordered_map<std::string, uint32_t> PixelTargets;
    std::unordered_map<uint32_t, std::string> PixelTargetsInverse;

    bool HasMaterial = false;
    std::vector<MaterialSchemeEntry> MaterialSchemes;
    BeShaderTools::RootLayout RootLayout;
};
