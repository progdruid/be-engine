#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <umbrellas/bitmask.hpp>
#include <umbrellas/include-glm.h>


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


// ─── blend state ───────────────────────────────────────────────
enum class SenBlendFactor : uint8_t {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstColor,
    InvDstColor,
    DstAlpha,
    InvDstAlpha,
};

enum class SenBlendOp : uint8_t {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

struct SenBlendState {
    bool              Enable           = false;
    SenBlendFactor    SrcBlend         = SenBlendFactor::One;
    SenBlendFactor    DstBlend         = SenBlendFactor::Zero;
    SenBlendOp        BlendOp          = SenBlendOp::Add;
    SenBlendFactor    SrcBlendAlpha    = SenBlendFactor::One;
    SenBlendFactor    DstBlendAlpha    = SenBlendFactor::Zero;
    SenBlendOp        BlendOpAlpha     = SenBlendOp::Add;
};


// ─── rasterizer state ──────────────────────────────────────────
enum class SenCullMode : uint8_t {
    None,
    Front,
    Back,
};

enum class SenFillMode : uint8_t {
    Solid,
    Wireframe,
};

struct SenRasterizerState {
    SenCullMode CullMode              = SenCullMode::Back;
    SenFillMode FillMode              = SenFillMode::Solid;
    float       DepthBias             = 0.f;
    float       SlopeScaledDepthBias  = 0.f;
    bool        DepthClipEnable       = true;
    bool        ScissorEnable         = false;
};


// ─── depth-stencil state ───────────────────────────────────────
enum class SenComparisonFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

struct SenDepthStencilState {
    bool              DepthEnable      = true;
    bool              DepthWriteEnable = true;
    SenComparisonFunc DepthFunc        = SenComparisonFunc::Less;
};


// ─── topology ───────────────────────────────────────────────────
enum class SenTopology : uint8_t {
    Undefined,
    TriangleList,
    TriangleStrip,
    LineList,
    PointList,
    PatchList3,
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

struct SenShaderSourceDesc {
    std::filesystem::path SourcePath;
    std::string FunctionName;
    SenShaderStage Stage;
};

// ─── vertex layout ─────────────────────────────────────────────
struct SenVertexLayoutElement {
    std::string Semantic;
    SenFormat Format;
    uint32_t Offset;
};

struct SenVertexLayoutDesc {
    std::vector<SenVertexLayoutElement> Elements;
};


// ─── pipeline ──────────────────────────────────────────────────
struct SenPipeline {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenPipelineDesc {
    // Shader stages
    SenShader VertexShader;
    SenShader HullShader;
    SenShader DomainShader;
    SenShader PixelShader;

    // Vertex input
    std::vector<SenVertexLayoutElement> VertexLayout;
    SenTopology Topology = SenTopology::TriangleList;

    // Render state
    SenRasterizerState    RasterizerState;
    SenBlendState         BlendState;
    SenDepthStencilState  DepthStencilState;

    // Optional: render target formats (for validation/compatibility checking)
    std::vector<SenFormat> RenderTargetFormats;
};


// ─── viewport ───────────────────────────────────────────────────
struct SenViewport {
    float X        = 0.f;
    float Y        = 0.f;
    float Width    = 0.f;
    float Height   = 0.f;
    float MinDepth = 0.f;
    float MaxDepth = 1.f;
};


// ─── render pass ───────────────────────────────────────────────
enum class SenLoadOp : uint8_t {
    Load,     // load existing contents of attachment
    Clear,    // clear attachment to clear value
    DontCare, // contents undefined, no load/clear needed (optimization)
};

struct SenColorAttachment {
    SenTexture Texture;
    uint8_t    MipLevel    = 0;
    int8_t     CubemapFace = -1;   // -1 = not a cubemap face, 0-5 = cubemap face index
    SenLoadOp  LoadOp      = SenLoadOp::Clear;
    glm::vec4  ClearColor  = {0, 0, 0, 0};
};

struct SenDepthAttachment {
    SenTexture Texture;
    int8_t     CubemapFace  = -1;   // -1 = not a cubemap face, 0-5 = cubemap face index
    SenLoadOp  LoadOp       = SenLoadOp::Clear;
    float      ClearDepth   = 1.0f;
    uint8_t    ClearStencil = 0;
};

struct SenPassDesc {
    std::vector<SenColorAttachment>   ColorAttachments;
    std::optional<SenDepthAttachment> DepthAttachment;
    SenViewport                       Viewport;
};


// ─── bind group ──────────────────────────────────────────────────
struct SenBindGroup {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenBindGroupDesc {
    struct TextureEntry {
        SenTexture Texture;
        uint8_t    Slot;
    };

    struct SamplerEntry {
        SenSampler Sampler;
        uint8_t    Slot;
    };

    struct BufferEntry {
        SenBuffer Buffer;
        uint8_t   Slot;
    };
    
    std::vector<TextureEntry> Textures;
    std::vector<SamplerEntry> Samplers;
    std::vector<BufferEntry>  ConstantBuffers;
};
