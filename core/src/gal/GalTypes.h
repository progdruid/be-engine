#pragma once
#include <cstdint>
#include <umbrellas/access-modifiers.hpp>

enum class GalFormat : uint32_t {
    Unknown = 0,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R16G16B16A16_FLOAT,
    R32G32B32A32_FLOAT,
    R32G32B32_FLOAT,
    R32G32_FLOAT,
    R11G11B10_FLOAT,
    R32_FLOAT,
    R16_FLOAT,
    R32_TYPELESS,
    R24G8_TYPELESS,
    D32_FLOAT,
    D24_UNORM_S8_UINT,
};

enum class GalTopology : uint8_t {
    Undefined = 0,
    TriangleList,
    TriangleStrip,
    PatchList3,
};

enum class GalBindFlags : uint32_t {
    None = 0,
    ShaderResource  = 1 << 0,
    RenderTarget    = 1 << 1,
    DepthStencil    = 1 << 2,
    ConstantBuffer  = 1 << 3,
    VertexBuffer    = 1 << 4,
    IndexBuffer     = 1 << 5,
};

enum class GalBufferUsage : uint8_t {
    Default = 0,
    Dynamic,
    Immutable,
    Staging,
};

enum class GalFilter : uint8_t {
    Point = 0,
    Linear,
    Anisotropic,
};

enum class GalAddressMode : uint8_t {
    Wrap = 0,
    Clamp,
    Mirror,
    Border,
};

enum class GalCullMode : uint8_t {
    None = 0,
    Back,
    Front,
};

enum class GalComparisonFunc : uint8_t {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

struct GalViewport {
    float X = 0;
    float Y = 0;
    float Width = 0;
    float Height = 0;
    float MinDepth = 0;
    float MaxDepth = 1;
};

constexpr auto operator|(GalBindFlags a, GalBindFlags b) -> GalBindFlags {
    return static_cast<GalBindFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr auto operator&(GalBindFlags a, GalBindFlags b) -> GalBindFlags {
    return static_cast<GalBindFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr auto HasFlag(GalBindFlags value, GalBindFlags flag) -> bool {
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}
