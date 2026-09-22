#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>


inline constexpr uint32_t SenMaxRootSize = 128;
inline constexpr uint32_t SenAllMips = UINT32_MAX;
inline constexpr uint32_t SenAllLayers = UINT32_MAX;


enum class SenHandleKind : uint8_t {
    Texture,
    View,
    Buffer,
    Sampler,
    Pipeline,
    Swapchain,
    CommandList,
};

template <SenHandleKind Kind>
struct SenHandle {
    uint32_t Index = 0;
    uint32_t Generation = 0;
    auto IsValid() const -> bool { return Generation != 0; }
    auto operator==(const SenHandle& other) const -> bool = default;
};

using SenTexture = SenHandle<SenHandleKind::Texture>;
using SenView = SenHandle<SenHandleKind::View>;
using SenBuffer = SenHandle<SenHandleKind::Buffer>;
using SenSampler = SenHandle<SenHandleKind::Sampler>;
using SenPipeline = SenHandle<SenHandleKind::Pipeline>;
using SenSwapchain = SenHandle<SenHandleKind::Swapchain>;
using SenCommandList = SenHandle<SenHandleKind::CommandList>;

struct SenGpuAddress {
    uint64_t Value = 0;
    auto IsValid() const -> bool { return Value != 0; }
    auto operator+(uint64_t offset) const -> SenGpuAddress { return { Value + offset }; }
};


enum class SenPlatform { Windows, Linux, Unknown };
constexpr SenPlatform SenCurrentPlatform =
#if defined(_WIN32) //
    SenPlatform::Windows;
#elif defined(__linux__) //
    SenPlatform::Linux;
#else //
    SenPlatform::Unknown;
#endif //


enum class SenMemory : uint8_t {
    Device,  // device-local, written through a copy
    Upload,  // host-visible, persistently mapped
};

enum class SenQueue : uint8_t {
    Graphics,
    Compute,   // falls back to the graphics queue when the device has no dedicated family
    Transfer,  // falls back to the graphics queue when the device has no dedicated family
};
inline constexpr uint32_t SenQueueCount = 3;

enum class SenTextureUsage : uint32_t {
    None = 0,
    Sampled = 1 << 0,
    Storage = 1 << 1,
    ColorTarget = 1 << 2,
    DepthTarget = 1 << 3,
}; ENABLE_BITMASK(SenTextureUsage);

enum class SenViewType : uint8_t {
    Texture2D,
    Texture2DArray,
    TextureCube,
    TextureCubeArray,
};

enum class SenLayout : uint8_t {
    Undefined,
    TransferDst,
    ShaderRead,
    Storage,
    ColorTarget,
    DepthTarget,
    Present,
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

enum class SenCompare : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class SenTopology : uint8_t {
    Undefined,
    TriangleList,
    TriangleStrip,
    LineList,
    PointList,
    PatchList3,
};

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

enum class SenCull : uint8_t {
    None,
    Front,
    Back,
};

enum class SenFill : uint8_t {
    Solid,
    Wireframe,
};

enum class SenFrontFace : uint8_t {
    Clockwise,
    CounterClockwise,
};

enum class SenLoadOp : uint8_t {
    Load,     // load existing contents of attachment
    Clear,    // clear attachment to clear value
    DontCare, // contents undefined, no load/clear needed (optimisation)
};

enum class SenPresentMode : uint8_t {
    Immediate,  // no vsync
    VSync,      // vsync
    Mailbox,    // triple-buf
};

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

inline auto SenGetFormatBytes(SenFormat format) -> uint32_t {
    switch (format) {
        case SenFormat::RGBA8_Unorm:     return 4;
        case SenFormat::BGRA8_Unorm:     return 4;
        case SenFormat::RGBA16_Float:    return 8;
        case SenFormat::R11G11B10_Float: return 4;
        case SenFormat::Depth32:         return 4;
        case SenFormat::RGB32_Float:     return 12;
        case SenFormat::RGBA32_Float:    return 16;
        case SenFormat::RG32_Float:      return 8;
        case SenFormat::R32_Float:       return 4;
        default:                         return 4;
    }
}


struct SenInitDesc {
    bool DebugLayer = false;
};

struct SenCaps {
    char DeviceName[256] = {};
    uint32_t TextureSlots = 0;
    uint32_t StorageSlots = 0;
    uint32_t SamplerSlots = 0;
    bool DedicatedCompute = false;
    bool DedicatedTransfer = false;
};

struct SenBufferDesc {
    SenMemory Memory = SenMemory::Device;
    uint32_t Size = 0;  // in bytes
};

struct SenTextureDesc {
    SenFormat Format = SenFormat::Unknown;
    uint32_t Width = 0;
    uint32_t Height = 0;
    SenTextureUsage Usage = SenTextureUsage::None;
    uint32_t Mips = 1;
    bool Cubemap = false;
    uint32_t ArrayLength = 1;
};

struct SenViewDesc {
    SenTexture Texture;
    SenViewType Type = SenViewType::Texture2D;
    uint32_t BaseMip = 0;
    uint32_t MipCount = 1;
    uint32_t BaseLayer = 0;
    uint32_t LayerCount = 1;
};

struct SenSamplerDesc {
    SenFilter Filter = SenFilter::Linear;
    SenAddressMode Address = SenAddressMode::Clamp;  // applied to U, V, and W
    bool Comparison = false;                         // enables less-than comparison (shadow maps)
};

struct SenShaderCode {
    const uint32_t* Code = nullptr;
    uint32_t Count = 0;
    auto IsValid() const -> bool { return Code != nullptr; }
};

struct SenVertexLayoutElement {
    std::string Semantic;  // HLSL semantic name, used by DX11
    uint32_t Location;     // SPIR-V location, used by Vulkan
    SenFormat Format;
    uint32_t Offset;
};

struct SenVertexLayoutDesc {
    std::vector<SenVertexLayoutElement> Elements;
};

struct SenBlendState {
    bool Enable = false;
    SenBlendFactor SrcBlend = SenBlendFactor::One;
    SenBlendFactor DstBlend = SenBlendFactor::Zero;
    SenBlendOp BlendOp = SenBlendOp::Add;
    SenBlendFactor SrcBlendAlpha = SenBlendFactor::One;
    SenBlendFactor DstBlendAlpha = SenBlendFactor::Zero;
    SenBlendOp BlendOpAlpha = SenBlendOp::Add;
};

struct SenDepthState {
    bool Test = true;
    bool Write = true;
    SenCompare Compare = SenCompare::Less;
};

struct SenPipelineDesc {
    SenShaderCode VertexShader;
    SenShaderCode HullShader;
    SenShaderCode DomainShader;
    SenShaderCode PixelShader;
    SenShaderCode ComputeShader;

    std::vector<SenVertexLayoutElement> VertexLayout;
    uint32_t VertexStride = 0;  // full stride of the vertex buffer (0 = no vertex input)
    SenTopology Topology = SenTopology::TriangleList;

    SenFill Fill = SenFill::Solid;
    bool DepthClipEnable = true;
    SenBlendState BlendState;

    std::vector<SenFormat> RenderTargetFormats;
    SenFormat DepthFormat;
};

struct SenViewport {
    float X = 0.f;
    float Y = 0.f;
    float Width = 0.f;
    float Height = 0.f;
    float MinDepth = 0.f;
    float MaxDepth = 1.f;
};

struct SenColorAttachment {
    SenView View;
    SenLoadOp LoadOp = SenLoadOp::Clear;
    glm::vec4 ClearColor = {0, 0, 0, 0};
};

struct SenDepthAttachment {
    SenView View;
    SenLoadOp LoadOp = SenLoadOp::Clear;
    float ClearDepth = 1.0f;
    uint8_t ClearStencil = 0;
};

struct SenRenderPassDesc {
    std::vector<SenColorAttachment> ColorAttachments;
    std::optional<SenDepthAttachment> DepthAttachment;
    SenViewport Viewport;
};

struct SenSubresource {
    uint32_t BaseMip = 0;
    uint32_t MipCount = SenAllMips;
    uint32_t BaseLayer = 0;
    uint32_t LayerCount = SenAllLayers;
};

struct SenTransition {
    SenTexture Texture;
    SenSubresource Subresource;
    SenLayout From = SenLayout::Undefined;
    SenLayout To = SenLayout::Undefined;
};

struct SenSwapchainDesc {
    void* NativeWindowHandle = nullptr;  // GLFWwindow* (TODO: see SenVulkanBackend.cpp)
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t BufferCount = 2;
    SenFormat Format = SenFormat::RGBA8_Unorm;
    SenPresentMode PresentMode = SenPresentMode::VSync;
};

struct SenSubmission {
    SenQueue Queue = SenQueue::Graphics;
    uint64_t Id = 0;
    auto IsValid() const -> bool { return Id != 0; }
};

struct SenSubmitDesc {
    SenQueue Queue = SenQueue::Graphics;
    const SenCommandList* Lists = nullptr;
    uint32_t ListCount = 0;
    const SenSubmission* Waits = nullptr;
    uint32_t WaitCount = 0;
    const SenSwapchain* Presents = nullptr;
    uint32_t PresentCount = 0;
};
