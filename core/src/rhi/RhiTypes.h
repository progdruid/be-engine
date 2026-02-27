#pragma once
#include <cstdint>
#include <umbrellas/access-modifiers.hpp>

enum class RhiFormat : uint32_t {
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

enum class RhiTopology : uint8_t {
    Undefined = 0,
    TriangleList,
    TriangleStrip,
    PatchList3,
};

enum class RhiBindFlags : uint32_t {
    None = 0,
    ShaderResource  = 1 << 0,
    RenderTarget    = 1 << 1,
    DepthStencil    = 1 << 2,
    ConstantBuffer  = 1 << 3,
    VertexBuffer    = 1 << 4,
    IndexBuffer     = 1 << 5,
};

enum class RhiBufferUsage : uint8_t {
    Default = 0,
    Dynamic,
    Immutable,
    Staging,
};

enum class RhiFilter : uint8_t {
    Point = 0,
    Linear,
    Anisotropic,
};

enum class RhiAddressMode : uint8_t {
    Wrap = 0,
    Clamp,
    Mirror,
    Border,
};

enum class RhiCullMode : uint8_t {
    None = 0,
    Back,
    Front,
};

enum class RhiComparisonFunc : uint8_t {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

struct RhiViewport {
    float X = 0;
    float Y = 0;
    float Width = 0;
    float Height = 0;
    float MinDepth = 0;
    float MaxDepth = 1;
};

constexpr auto operator|(RhiBindFlags a, RhiBindFlags b) -> RhiBindFlags {
    return static_cast<RhiBindFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr auto operator&(RhiBindFlags a, RhiBindFlags b) -> RhiBindFlags {
    return static_cast<RhiBindFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr auto HasFlag(RhiBindFlags value, RhiBindFlags flag) -> bool {
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}
