#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <umbrellas/bitmask.hpp>


enum class SenFormat : uint8_t {
    Unknown,
    RGBA8_Unorm,
    RGBA16_Float,
    R11G11B10_Float,
    Depth32,
    RGB32_Float,
    RGBA32_Float,
    RG32_Float,
};

// ─── texture ─────────────────────────────────────────────────────
enum class SenTextureUsage : uint32_t {
    None           = 0,
    ShaderResource = 1 << 0,
    RenderTarget   = 1 << 1,
    DepthStencil   = 1 << 2,
};
ENABLE_BITMASK(SenTextureUsage);

struct SenTexture {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenTextureDesc {
    SenFormat       Format  = SenFormat::Unknown;
    uint32_t        Width   = 0;
    uint32_t        Height  = 0;
    SenTextureUsage Usage   = SenTextureUsage::None;
    uint32_t        Mips    = 1;
    bool            Cubemap = false;
    const uint8_t*  Data    = nullptr; // optional initial pixel data, not owned
};



// ─── buffer ─────────────────────────────────────────────────────
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

struct SenBuffer {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenBufferDesc {
    SenBufferUsage  Usage  = SenBufferUsage::Constant;
    SenBufferAccess Access = SenBufferAccess::Dynamic;
    uint32_t        Size   = 0;       // in bytes
    const void*     Data   = nullptr; // optional initial data, not owned
};


// ─── sampler ─────────────────────────────────────────────────────
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

struct SenSampler {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenSamplerDesc {
    SenFilter      Filter     = SenFilter::Linear;
    SenAddressMode Address    = SenAddressMode::Clamp; // applied to U, V, and W
    bool           Comparison = false;                 // enables less-than comparison (shadow maps)
};


// ─── shader ─────────────────────────────────────────────────────
enum class SenShaderStage : uint8_t {
    Vertex,
    Hull,
    Domain,
    Pixel,
};

struct SenShader {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenShaderDesc {
    const void* Blob = nullptr;
    uint32_t BlobSize = 0;
    SenShaderStage Stage;
};

// ─── vertex layout ─────────────────────────────────────────────
struct SenVertexLayout {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenVertexLayoutDesc {
    struct Element {
        std::string Semantic;
        SenFormat Format;
        uint32_t Offset;
    };
    std::vector<Element> Elements;
    const void* VertexShaderBytecode = nullptr;     // raw pointer to shader bytecode
    uint32_t VertexShaderBytecodeSize = 0;          // bytecode size in bytes
};


// ─── other ─────────────────────────────────────────────────────
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
