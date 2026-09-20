#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>



// ─── handles ──────────────────────────────────────────────────────
enum class SenHandleKind : uint8_t {
    Texture,
    Buffer,
    Sampler,
    Pipeline,
    Swapchain,
};

template <SenHandleKind Kind>
struct SenHandle {
    uint32_t Index = 0;
    uint32_t Generation = 0;
    auto IsValid() const -> bool { return Generation != 0; }
    auto operator==(const SenHandle& other) const -> bool = default;
};


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
    R32_Float,
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

enum class SenHeapBinding : uint32_t {
    Texture2D        = 0,
    Texture2DArray   = 1,
    TextureCube      = 2,
    TextureCubeArray = 3,
    StorageTexture2D = 4,
    Sampler          = 5,
    Count            = 6,
};

enum class SenResourceState : uint8_t {
    Undefined,
    ShaderRead,
    ColorAttachment,
    DepthAttachment,
    TransferDst,
    Present,
    UnorderedAccess,
};

constexpr uint32_t SEN_FULL_MIPS = UINT32_MAX;

using SenTexture = SenHandle<SenHandleKind::Texture>;

struct SenTextureDesc {
    SenFormat       Format      = SenFormat::Unknown;
    uint32_t        Width       = 0;
    uint32_t        Height      = 0;
    SenTextureUsage Usage       = SenTextureUsage::None;
    uint32_t        Mips        = 1;
    bool            Cubemap     = false;
    uint32_t        ArrayLength = 1;
    const uint8_t*  Data        = nullptr; // optional initial pixel data, not owned
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

using SenBuffer = SenHandle<SenHandleKind::Buffer>;

struct SenBufferGpuAddress {
    uint64_t Value = 0;
    auto IsValid() const -> bool { return Value != 0; }
    auto operator+(uint64_t offset) const -> SenBufferGpuAddress { return { Value + offset }; }
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

using SenSampler = SenHandle<SenHandleKind::Sampler>;

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


// ─── submission ─────────────────────────────────────────────────
struct SenSubmission {
    uint64_t Value = 0;
    auto IsValid() const -> bool { return Value != 0; }
};


// ─── shader ─────────────────────────────────────────────────────
struct SenShaderCode {
    const uint32_t* Code = nullptr;
    uint32_t Count = 0;
    auto IsValid() const -> bool { return Code != nullptr; }
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
using SenPipeline = SenHandle<SenHandleKind::Pipeline>;

inline constexpr uint32_t SenMaxRootConstantSize = 128;

struct SenPipelineDesc {
    // Shader stages
    SenShaderCode VertexShader;
    SenShaderCode HullShader;
    SenShaderCode DomainShader;
    SenShaderCode PixelShader;
    SenShaderCode ComputeShader;

    // Vertex input
    std::vector<SenVertexLayoutElement> VertexLayout;
    uint32_t    VertexStride = 0;  // full stride of the vertex buffer (0 = no vertex input)
    SenTopology Topology = SenTopology::TriangleList;

    // Render state
    SenRasterizerState    RasterizerState;
    SenBlendState         BlendState;
    SenDepthStencilState  DepthStencilState;

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
    int16_t    Layer       = -1;   // -1 = whole texture, >=0 = array/cube layer index
    SenLoadOp  LoadOp      = SenLoadOp::Clear;
    glm::vec4  ClearColor  = {0, 0, 0, 0};
};

struct SenDepthAttachment {
    SenTexture Texture;
    int16_t    Layer        = -1;   // -1 = whole texture, >=0 = array/cube layer index
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

using SenSwapchain = SenHandle<SenHandleKind::Swapchain>;

struct SenSwapchainDesc {
    void*          NativeWindowHandle = nullptr;  // GLFWwindow* (TODO: see SenVulkanBackend.cpp)
    uint32_t       Width          = 0;
    uint32_t       Height         = 0;
    uint32_t       BufferCount    = 2;
    uint32_t       FramesInFlight = 2;
    SenFormat      Format         = SenFormat::RGBA8_Unorm;
    SenPresentMode PresentMode    = SenPresentMode::VSync;
};

