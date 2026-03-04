#pragma once
#include <cstdint>
#include <umbrellas/bitmask.hpp>

enum class SenFormat : uint8_t {
    Unknown,
    RGBA8_Unorm,
    RGBA16_Float,
    R11G11B10_Float,
    Depth32,
};

enum class SenTextureUsage : uint32_t {
    None           = 0,
    ShaderResource = 1 << 0,
    RenderTarget   = 1 << 1,
    DepthStencil   = 1 << 2,
};
ENABLE_BITMASK(SenTextureUsage);

enum class SenTopology : uint8_t {
    Undefined,
    TriangleList,
    TriangleStrip,
    LineList,
    PointList,
    PatchList3,
};

struct SenViewport {
    float X        = 0.f;
    float Y        = 0.f;
    float Width    = 0.f;
    float Height   = 0.f;
    float MinDepth = 0.f;
    float MaxDepth = 1.f;
};
