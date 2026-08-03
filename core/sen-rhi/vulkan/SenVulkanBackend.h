#pragma once
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

#include "sen-rhi/SenCommandBuffer.h"
#include "sen-rhi/SenTypes.h"
#include <umbrellas/common.hpp>


struct ISlangBlob;
namespace Slang { template <typename T> class ComPtr; }

// ─── resource entries ─────────────────────────────────────────────────────────

struct SenVulkanTextureEntry {
    VkImage Image = VK_NULL_HANDLE;
    VmaAllocation Allocation = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageView SRV = VK_NULL_HANDLE;                       // for shader sampling (all mips, all layers)
    VkImageView DSV = VK_NULL_HANDLE;                       // depth attachment (2D or per-face for cubemap — see below)
    std::vector<VkImageView> MipSRVs;                       // [mip]       — single-mip sampling view (2D)
    std::vector<VkImageView> MipRTVs;                       // [mip]       — color attachment per mip (2D)
    std::array<VkImageView, 6> CubemapDSVs  = {};           // [face]      — depth attachment per cubemap face
    std::array<std::vector<VkImageView>, 6> CubemapMipRTVs; // [face][mip] — color attachment per cubemap face per mip
    std::vector<VkImageLayout> MipLayouts;                  // per-mip current layout (all faces share a mip's layout)
    uint32_t Width      = 0;                                // mip-0 dimensions, kept for mip generation
    uint32_t Height     = 0;
    uint32_t MipLevels  = 1;
    uint32_t LayerCount = 1;                                // 1 for 2D, 6 for cubemaps
};

struct SenVulkanBufferEntry {
    VkBuffer        Buffer     = VK_NULL_HANDLE;
    VmaAllocation   Allocation = VK_NULL_HANDLE;
    SenBufferAccess Access     = SenBufferAccess::Dynamic;
    uint32_t        Size       = 0;
    void*           MappedPtr  = nullptr;  // non-null for Dynamic buffers (persistently mapped)
};

struct SenVulkanSamplerEntry {
    VkSampler Sampler = VK_NULL_HANDLE;
};

struct SenVulkanShaderEntry {
    VkShaderModule Module = VK_NULL_HANDLE;
    SenShaderStage Stage;
    std::filesystem::path SourcePath;
    std::string FunctionName;
    std::vector<std::filesystem::path> Includes;
};


struct SenVulkanPipelineEntry {
    VkPipeline Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout Layout = VK_NULL_HANDLE;
    VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SenPipelineDesc Desc;
};

struct SenVulkanBindGroupEntry {
    VkDescriptorSet Set = VK_NULL_HANDLE;
    SenBindGroupDesc BindGroupDesc;
};

struct SenVulkanSwapchainEntry {
    VkSurfaceKHR   Surface   = VK_NULL_HANDLE;
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     Images;
    std::vector<VkImageView> ImageViews;
    std::vector<SenTexture>  Textures;   // SenTexture handle per swapchain image

    // per frame slot
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkFence>     InFlightFences;

    // per swapchain image
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence>     ImagesInFlight;

    uint32_t CurrentImageIndex = 0;

    void* NativeWindowHandle;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t BufferCount;
    uint32_t FramesInFlight = 2;
    SenFormat Format;
    SenPresentMode PresentMode;
};

// ─── backend ──────────────────────────────────────────────────────────────────

class SenVulkanBackend {
    hide
    static VkInstance _instance;
    static VkPhysicalDevice _physicalDevice;
    static VkDevice _device;
    static VkQueue _queue;
    static uint32_t _queueFamilyIndex;
    static uint32_t _minUniformBufferOffsetAlignment;
    static VkCommandPool _commandPool;
    static VkDescriptorPool _descriptorPool;
    static VmaAllocator _allocator;

    hide
    static std::unordered_map<uint32_t, SenVulkanSwapchainEntry> _swapchains;   static uint32_t _nextSwapchainId;
    static std::unordered_map<uint32_t, SenVulkanTextureEntry> _textures;       static uint32_t _nextTextureId;
    static std::unordered_map<uint32_t, SenVulkanBufferEntry> _buffers;         static uint32_t _nextBufferId;
    static std::unordered_map<uint32_t, SenVulkanSamplerEntry> _samplers;       static uint32_t _nextSamplerId;
    static std::unordered_map<uint32_t, SenVulkanBindGroupEntry> _bindGroups;   static uint32_t _nextBindGroupId;
    static std::unordered_map<uint32_t, SenVulkanShaderEntry> _shaders;         static uint32_t _nextShaderId;
    static std::unordered_map<uint32_t, SenVulkanPipelineEntry> _pipelines;     static uint32_t _nextPipelineId;
    
    expose
    static auto Init      (const SenDeviceDesc& desc) -> void;
    static auto Shutdown  () -> void;
    static auto WaitIdle  () -> void;

    static auto GetMinUniformBufferOffsetAlignment () -> uint32_t { return _minUniformBufferOffsetAlignment; }
    
    expose // swapchain lifecycle
    static auto CreateSwapchain       (const SenSwapchainDesc& desc) -> SenSwapchain;
    static auto DestroySwapchain      (SenSwapchain handle) -> void;
    static auto ResizeSwapchain       (SenSwapchain& handle, uint32_t width, uint32_t height) -> void;
    static auto BeginFrame            (SenSwapchain handle, uint32_t frameSlot) -> SenTexture;
    static auto EndFrame              (SenSwapchain handle, SenVulkanCommandBuffer& cmd, uint32_t frameSlot) -> void;
    static auto GetSwapchainFormat    (SenSwapchain handle) -> SenFormat;
    static auto GetSwapchainWidth     (SenSwapchain handle) -> uint32_t;
    static auto GetSwapchainHeight    (SenSwapchain handle) -> uint32_t;

    expose // command buffer factory
    static auto AllocateCommandBuffer () -> SenVulkanCommandBuffer;
    static auto SubmitImmediate       (SenVulkanCommandBuffer& cmd) -> void;  // submit + fence-wait, no swapchain sync

    expose // native API escape hatches (for ImGui, etc.)
    static auto GetNativeDevice          () -> void*;  // VkDevice
    static auto GetNativeInstance        () -> void*;  // VkInstance
    static auto GetNativePhysicalDevice  () -> void*;  // VkPhysicalDevice
    static auto GetNativeQueue           () -> void*;  // VkQueue
    static auto GetNativeQueueFamilyIndex() -> uint32_t;

    expose // debug annotation helpers
    static auto BeginDebugEvent (const std::string& label) -> void;
    static auto EndDebugEvent   () -> void;
    
    expose // textures
    static auto CreateTexture  (const SenTextureDesc& desc) -> SenTexture;
    static auto DestroyTexture (SenTexture handle) -> void;
    static auto LookupTexture  (SenTexture handle) -> SenVulkanTextureEntry&;
    static auto GenerateMips   (SenTexture handle) -> void;   // blit-chain downsample of mip 0 into the rest
    hide static auto CreateImageView      (VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipLevels, uint32_t baseLayer, uint32_t layerCount) -> VkImageView;
    expose static auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) -> VkImageMemoryBarrier2;
    expose static auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout) -> VkImageMemoryBarrier2;
    expose static auto RecordImageBarrier(VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier) -> void;
    hide static auto UploadToDeviceImage  (VkImage image, VkImageAspectFlags aspect, const void* data, uint32_t dataSize, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layerCount) -> void;
    hide static auto CreateTexture2D      (const SenTextureDesc& desc, SenVulkanTextureEntry& entry) -> void;
    hide static auto CreateTextureCubemap (const SenTextureDesc& desc, SenVulkanTextureEntry& entry) -> void;
    
    expose // buffers
    static auto CreateBuffer  (const SenBufferDesc& desc) -> SenBuffer;
    static auto DestroyBuffer (SenBuffer handle) -> void;
    static auto LookupBuffer  (SenBuffer handle) -> SenVulkanBufferEntry&;
    static auto WriteBuffer   (SenBuffer handle, const void* data, uint32_t size, uint32_t dstOffset = 0) -> void;
    hide static auto UploadToDeviceBuffer(VkBuffer dst, const void* data, uint32_t size, uint32_t dstOffset) -> void;

    expose // samplers
    static auto CreateSampler  (const SenSamplerDesc& desc) -> SenSampler;
    static auto DestroySampler (SenSampler handle) -> void;
    static auto LookupSampler  (SenSampler handle) -> SenVulkanSamplerEntry&;

    expose // bind groups
    static auto CreateBindGroup  (const SenBindGroupDesc& desc) -> SenBindGroup;
    static auto DestroyBindGroup (SenBindGroup handle) -> void;
    static auto LookupBindGroup  (SenBindGroup handle) -> SenVulkanBindGroupEntry&;
    hide static auto CreateDescriptorSetLayoutFromDesc(const SenBindGroupDesc& desc) -> VkDescriptorSetLayout;
    
    expose // shaders
    static auto CreateShader (const SenShaderSourceDesc& sourceDesc) -> SenShader;
    static auto DestroyShader (SenShader handle) -> void;
    static auto LookupShader  (SenShader handle) -> SenVulkanShaderEntry&;
    static auto ReloadSources (std::span<const std::filesystem::path> paths) -> void;
    hide static auto ReloadShader(SenShader handle) -> bool;

    expose // pipelines
    static auto CreatePipeline  (const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline (SenPipeline handle) -> void;
    static auto LookupPipeline  (SenPipeline handle) -> SenVulkanPipelineEntry&;
    hide static auto MakePipelineEntry(const SenPipelineDesc& desc) -> SenVulkanPipelineEntry;
    hide static auto ReloadPipeline(SenPipeline handle) -> void;

    expose // debug print helpers
    static auto PrintBindGroup (SenBindGroup handle) -> std::string;
};
