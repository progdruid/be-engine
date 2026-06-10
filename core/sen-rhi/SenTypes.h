#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <umbrellas/bitmask.hpp>
#include <umbrellas/include-glm.h>



// ─── platform ─────────────────────────────────────────────────────
enum class SenPlatform { Windows, Linux, Unknown };

constexpr SenPlatform SenCurrentPlatform =
#if defined(_WIN32) // 
    SenPlatform::Windows;
#elif defined(__linux__) //  
    SenPlatform::Linux;
#else //
    SenPlatform::Unknown;
#endif //


enum class SenFormat : uint8_t {
    Unknown,
    RGBA8_Unorm,
    BGRA8_Unorm,
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
    Storage        = 1 << 3,
};
ENABLE_BITMASK(SenTextureUsage);

enum class SenResourceState : uint8_t {
    Undefined,
    ShaderRead,
    ColorAttachment,
    DepthAttachment,
    TransferDst,
    Present,
    UnorderedAccess,
};

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
    Storage,
};

enum class SenBufferAccess : uint8_t {
    Immutable, // set at creation, never written again - Vulkan: device-local via staging, assert write - DX11: USAGE_IMMUTABLE
    Static,    // rarely updated (not per-frame)       - Vulkan: device-local via staging               - DX11: UpdateSubresource
    Dynamic,   // written every frame                  - Vulkan: host-visible persistent map            - DX11: Map/Unmap DISCARD
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


// ─── bind group layout ───────────────────────────────────────────
enum class SenShaderStageFlags : uint8_t {
    None = 0,
    Vertex = 1 << 0,
    Pixel  = 1 << 1,
    Hull = 1 << 2,
    Domain = 1 << 3,
    Compute = 1 << 4,
    AllGraphics = Vertex | Pixel | Hull | Domain,
};
ENABLE_BITMASK(SenShaderStageFlags);

// Note: SenBindGroupLayout no longer exists as a handle type.
// Descriptor set layouts are created on-demand from SenBindGroupDesc in the backend.


// ─── bind group ──────────────────────────────────────────────────
struct SenBindGroup {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenBindGroupDesc {
    SenShaderStageFlags Stages = SenShaderStageFlags::AllGraphics;
    std::vector<uint8_t> BufferSlots;
    std::vector<uint8_t> SamplerSlots;
    std::vector<uint8_t> TextureSlots;
    std::vector<uint8_t> StorageTextureSlots;
    std::vector<uint8_t> StorageBufferSlots;
    std::vector<SenBuffer> Buffers = {};
    std::vector<SenSampler> Samplers = {};
    std::vector<SenTexture> Textures = {};
    std::vector<SenTexture> StorageTextures = {};
    std::vector<SenBuffer> StorageBuffers = {};
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
    Compute,
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
    std::string Semantic;  // HLSL semantic name, used by DX11
    uint32_t    Location;  // SPIR-V location, used by Vulkan
    SenFormat   Format;
    uint32_t    Offset;
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
    SenShader ComputeShader;

    // Vertex input
    std::vector<SenVertexLayoutElement> VertexLayout;
    uint32_t    VertexStride = 0;  // full stride of the vertex buffer (0 = no vertex input)
    SenTopology Topology = SenTopology::TriangleList;

    // Render state
    SenRasterizerState    RasterizerState;
    SenBlendState         BlendState;
    SenDepthStencilState  DepthStencilState;

    // Bind group layouts (ordered by set index: 0, 1, 2, ...)
    // Pipeline only cares about structure (slot vectors), not actual resources
    std::vector<SenBindGroupDesc> BindGroupLayouts;

    std::vector<SenFormat> RenderTargetFormats;
    SenFormat DepthStencilFormat;
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


// ─── device ─────────────────────────────────────────────────────

struct SenDeviceDesc {
    bool DebugLayer = false;
};


// ─── swapchain ──────────────────────────────────────────────────

enum class SenPresentMode : uint8_t {
    Immediate,  // no vsync  — DX11: Present(0), Vulkan: IMMEDIATE_KHR
    VSync,      // vsync     — DX11: Present(1), Vulkan: FIFO_KHR
    Mailbox,    // triple-buf — Vulkan: MAILBOX_KHR, DX11: falls back to VSync
};

struct SenSwapchain {
    uint32_t ID = 0;
    auto IsValid() const -> bool { return ID != 0; }
};

struct SenSwapchainDesc {
    void*          NativeWindowHandle = nullptr;  // GLFWwindow* (TODO: see SenVulkanBackend.cpp)
    uint32_t       Width       = 0;
    uint32_t       Height      = 0;
    uint32_t       BufferCount = 2;
    SenFormat      Format      = SenFormat::RGBA8_Unorm;
    SenPresentMode PresentMode = SenPresentMode::VSync;
};

