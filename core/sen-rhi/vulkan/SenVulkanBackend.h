#pragma once
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

#include "sen-rhi/SenCommandBuffer.h"
#include "sen-rhi/SenSlotMap.h"
#include "sen-rhi/SenTypes.h"
#include <umbrellas/common.hpp>


struct ISlangBlob;
namespace Slang { template <typename T> class ComPtr; }

// ─── resource entries ─────────────────────────────────────────────────────────

struct SenVulkanTextureEntry {
    VkImage Image = VK_NULL_HANDLE;
    VmaAllocation Allocation = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    VkImageView SRV = VK_NULL_HANDLE;                   // for shader sampling (all mips, all layers)
    VkImageView DSV = VK_NULL_HANDLE;                   // depth attachment (2D or per-face for cubemap — see below)
    std::vector<VkImageView> MipSRVs;                   // [mip]        — single-mip sampling view (2D)
    std::vector<VkImageView> MipRTVs;                   // [mip]        — color attachment per mip (2D)
    std::vector<VkImageView> LayerDSVs;                 // [layer]      — depth attachment per array/cube image layer
    std::vector<std::vector<VkImageView>> LayerMipRTVs; // [layer][mip] — colour attachment per image layer per mip
    std::vector<VkImageLayout> MipLayouts;                  
    uint32_t Width      = 0;                            // mip-0 dimensions, kept for mip generation
    uint32_t Height     = 0;
    uint32_t MipLevels  = 1;
    uint32_t LayerCount = 1;                            // 1 for 2D, N for 2D array, 6 for cube, 6*N for cube array
    uint32_t HeapBinding = UINT32_MAX;
    uint32_t HeapIndex   = UINT32_MAX;
    std::vector<uint32_t> MipHeapIndices;               // [mip] — per-mip sampling slots (2D)
};

struct SenVulkanBufferEntry {
    VkBuffer      Buffer     = VK_NULL_HANDLE;
    VmaAllocation Allocation = VK_NULL_HANDLE;
    SenMemory     Memory     = SenMemory::Device;
    uint32_t      Size       = 0;
    void*         MappedPtr  = nullptr;  // non-null for Upload buffers (persistently mapped)
    uint64_t      GpuAddress = 0;        // cached device address
};

struct SenVulkanSamplerEntry {
    VkSampler Sampler = VK_NULL_HANDLE;
    uint32_t HeapIndex = UINT32_MAX;
};

struct SenVulkanPipelineEntry {
    VkPipeline Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout Layout = VK_NULL_HANDLE;
    VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

struct SenVulkanSwapchainEntry {
    VkSurfaceKHR   Surface   = VK_NULL_HANDLE;
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     Images;
    std::vector<VkImageView> ImageViews;
    std::vector<SenTexture>  Textures;   // SenTexture handle per swapchain image

    // per frame slot
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<uint64_t>    SlotTimelineValues;

    // per swapchain image
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<uint64_t>    ImageTimelineValues;

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
    static VkSemaphore _timeline;
    static uint64_t _timelineValue;
    static VkCommandPool _commandPool;
    static VmaAllocator _allocator;

    hide
    static SenSlotMap<SenVulkanSwapchainEntry, SenSwapchain> _swapchains;
    static SenSlotMap<SenVulkanTextureEntry, SenTexture> _textures;
    static SenSlotMap<SenVulkanBufferEntry, SenBuffer> _buffers;
    static SenSlotMap<SenVulkanSamplerEntry, SenSampler> _samplers;
    static SenSlotMap<SenVulkanPipelineEntry, SenPipeline> _pipelines;

    hide
    static VkDescriptorSetLayout _bindlessLayout;
    static VkDescriptorPool      _bindlessPool;
    static VkDescriptorSet       _bindlessSet;
    static std::array<uint32_t,              static_cast<size_t>(SenHeapBinding::Count)> _heapNext;
    static std::array<std::vector<uint32_t>, static_cast<size_t>(SenHeapBinding::Count)> _heapFree;
    static std::array<uint32_t,              static_cast<size_t>(SenHeapBinding::Count)> _heapCapacity;

    expose
    static auto Init      (const SenDeviceDesc& desc) -> void;
    static auto Shutdown  () -> void;
    static auto WaitIdle  () -> void;
    static auto IsSubmissionComplete(SenSubmission submission) -> bool;
    
    expose // swapchain lifecycle
    static auto CreateSwapchain       (const SenSwapchainDesc& desc) -> SenSwapchain;
    static auto DestroySwapchain      (SenSwapchain handle) -> void;
    static auto ResizeSwapchain       (SenSwapchain& handle, uint32_t width, uint32_t height) -> void;
    static auto BeginFrame            (SenSwapchain handle, uint32_t frameSlot) -> SenTexture;
    static auto EndFrame              (SenSwapchain handle, SenVulkanCommandBuffer& cmd, uint32_t frameSlot) -> SenSubmission;
    static auto GetSwapchainFormat    (SenSwapchain handle) -> SenFormat;
    static auto GetSwapchainWidth     (SenSwapchain handle) -> uint32_t;
    static auto GetSwapchainHeight    (SenSwapchain handle) -> uint32_t;
    static auto GetSurfaceExtent      (SenSwapchain handle, uint32_t& outWidth, uint32_t& outHeight) -> void;

    expose // command buffer factory
    static auto AllocateCommandBuffer () -> SenVulkanCommandBuffer;
    static auto SubmitImmediate       (SenVulkanCommandBuffer& cmd) -> SenSubmission;  // submit + fence-wait, no swapchain sync

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
    hide static auto CreateImageView      (VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipLevels, uint32_t baseLayer, uint32_t layerCount) -> VkImageView;
    expose static auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) -> VkImageMemoryBarrier2;
    expose static auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout) -> VkImageMemoryBarrier2;
    expose static auto RecordImageBarrier(VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier) -> void;
    
    expose // buffers
    static auto CreateBuffer  (const SenBufferDesc& desc) -> SenBuffer;
    static auto DestroyBuffer (SenBuffer handle) -> void;
    static auto LookupBuffer  (SenBuffer handle) -> SenVulkanBufferEntry&;
    static auto GetBufferGpuAddress(SenBuffer handle) -> SenBufferGpuAddress;
    static auto GetBufferMemory(SenBuffer handle) -> SenMemory;
    static auto GetBufferPointer(SenBuffer handle) -> void*;

    expose // samplers
    static auto CreateSampler  (const SenSamplerDesc& desc) -> SenSampler;
    static auto DestroySampler (SenSampler handle) -> void;
    static auto LookupSampler  (SenSampler handle) -> SenVulkanSamplerEntry&;

    expose // bindless heap
    static auto GetTextureHeapIndex (SenTexture handle, uint32_t mip = SEN_FULL_MIPS) -> uint32_t;
    static auto GetSamplerHeapIndex (SenSampler handle) -> uint32_t;
    static auto GetBindlessSet      () -> VkDescriptorSet { return _bindlessSet; }
    static auto GetBindlessLayout   () -> VkDescriptorSetLayout { return _bindlessLayout; }
    hide static auto InitBindlessHeap     () -> void;
    hide static auto ShutdownBindlessHeap () -> void;
    hide static auto HeapRegisterView     (SenHeapBinding binding, VkImageView view) -> uint32_t;
    hide static auto HeapRegisterTexture  (SenVulkanTextureEntry& entry, VkImageView view, VkImageViewType viewType) -> void;
    hide static auto HeapRegisterSampler  (SenVulkanSamplerEntry& entry) -> void;
    hide static auto HeapReleaseTexture   (SenVulkanTextureEntry& entry) -> void;
    hide static auto HeapReleaseSampler   (SenVulkanSamplerEntry& entry) -> void;
    hide static auto HeapAllocSlot        (SenHeapBinding binding) -> uint32_t;

    expose // pipelines
    static auto CreatePipeline  (const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline (SenPipeline handle) -> void;
    static auto LookupPipeline  (SenPipeline handle) -> SenVulkanPipelineEntry&;
};
