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

enum class SenBufferUsage : uint8_t {
    Vertex,
    Index,
    Constant,
};

enum class SenBufferAccess : uint8_t {
    Immutable, // upload once at creation, never written again      — DX11: USAGE_IMMUTABLE,   Vulkan: device-local via staging
    Default,   // written rarely (once or a few times per lifetime) — DX11: UpdateSubresource, Vulkan: device-local via staging
    Dynamic,   // written every frame                               — DX11: Map/Unmap DISCARD, Vulkan: host-visible buffer
};

enum class SenFilter : uint8_t {
    Point,
    Linear,
    Anisotropic,
};

enum class SenAddressMode : uint8_t {
    Wrap,
    Clamp,
    Mirror,
};

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
