#pragma once
#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

#include "Sen.h"
#include "SenSlotMap.h"
#include "SenTypes.h"


// --- resource entries ---------------------------------------------------------

struct SenVulkanTextureEntry {
    VkImage Image = VK_NULL_HANDLE;
    VmaAllocation Allocation = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    uint32_t Width      = 0;                            // mip-0 dimensions
    uint32_t Height     = 0;
    uint32_t MipLevels  = 1;
    uint32_t LayerCount = 1;                            // 1 for 2D, N for 2D array, 6 for cube, 6*N for cube array
};

struct SenVulkanViewEntry {
    VkImageView View = VK_NULL_HANDLE;
    SenViewDesc Desc;
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
};

struct SenVulkanPipelineEntry {
    VkPipeline Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout Layout = VK_NULL_HANDLE;
    VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

struct SenVulkanSwapchainEntry {
    VkSurfaceKHR   Surface   = VK_NULL_HANDLE;
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>    Images;
    std::vector<SenTexture> Textures;   // SenTexture handle per swapchain image
    std::vector<SenView>    Views;      // attachment view per swapchain image

    // acquire ring
    std::vector<VkSemaphore> AcquireSemaphores;
    std::vector<uint64_t>    AcquireTimelineValues;
    uint32_t AcquireIndex = 0;
    uint32_t PendingAcquireIndex = 0;

    // per swapchain image
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<uint64_t>    ImageTimelineValues;

    uint32_t CurrentImageIndex = 0;

    SenNativeWindow NativeWindow;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t BufferCount;
    SenFormat Format;
    SenPresentMode PresentMode;
};

struct SenVulkanCommandListEntry {
    SenQueue         Queue               = SenQueue::Graphics;
    VkCommandBuffer  Cmd                 = VK_NULL_HANDLE;
    VkPipelineLayout BoundPipelineLayout = VK_NULL_HANDLE;
    SenPipeline      BoundPipeline;
    bool             PipelineDirty       = false;

    SenCull Cull = SenCull::Back;
    SenFrontFace FrontFace = SenFrontFace::Clockwise;
    SenDepthState Depth;
    float DepthBiasConstant = 0.f;
    float DepthBiasSlope = 0.f;

    bool CullDirty = true;
    bool FrontFaceDirty = true;
    bool DepthDirty = true;
    bool DepthBiasDirty = true;
};

struct SenVulkanQueueSlot {
    VkQueue Queue = VK_NULL_HANDLE;
    uint32_t FamilyIndex = 0;
    VkCommandPool Pool = VK_NULL_HANDLE;
    VkSemaphore Timeline = VK_NULL_HANDLE;
    uint64_t Counter = 0;
    bool OwnsFamily = false;
};

// --- backend state ------------------------------------------------------------

namespace SenVk {

inline VkInstance _instance = VK_NULL_HANDLE;
inline VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
inline VkDevice _device = VK_NULL_HANDLE;
inline VmaAllocator _allocator = VK_NULL_HANDLE;
inline std::array<SenVulkanQueueSlot, SenQueueCount> _queues;

inline SenSlotMap<SenVulkanSwapchainEntry, SenSwapchain> _swapchains;
inline SenSlotMap<SenVulkanTextureEntry, SenTexture> _textures;
inline SenSlotMap<SenVulkanViewEntry, SenView> _views;
inline SenSlotMap<SenVulkanBufferEntry, SenBuffer> _buffers;
inline SenSlotMap<SenVulkanSamplerEntry, SenSampler> _samplers;
inline SenSlotMap<SenVulkanPipelineEntry, SenPipeline> _pipelines;
inline SenSlotMap<SenVulkanCommandListEntry, SenCommandList> _commandLists;

inline constexpr uint32_t BindlessTextureSlots = 4096;
inline constexpr uint32_t BindlessStorageSlots = 256;
inline constexpr uint32_t BindlessSamplerSlots = 256;

inline VkDescriptorSetLayout _bindlessLayout = VK_NULL_HANDLE;
inline VkDescriptorPool      _bindlessPool   = VK_NULL_HANDLE;
inline VkDescriptorSet       _bindlessSet    = VK_NULL_HANDLE;

// --- backend internals --------------------------------------------------------

auto LookupBuffer(SenBuffer handle) -> SenVulkanBufferEntry&;
auto LookupTexture(SenTexture handle) -> SenVulkanTextureEntry&;
auto LookupView(SenView handle) -> SenVulkanViewEntry&;
auto LookupSampler(SenSampler handle) -> SenVulkanSamplerEntry&;
auto LookupPipeline(SenPipeline handle) -> SenVulkanPipelineEntry&;
auto LookupCommandList(SenCommandList handle) -> SenVulkanCommandListEntry&;

auto CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipLevels, uint32_t baseLayer, uint32_t layerCount) -> VkImageView;
auto InitBindless() -> void;
auto ShutdownBindless() -> void;

auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) -> VkImageMemoryBarrier2;
auto MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout) -> VkImageMemoryBarrier2;
auto RecordImageBarrier(VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier) -> void;

auto FlushPipeline(SenVulkanCommandListEntry& entry) -> void;
auto FlushDrawState(SenVulkanCommandListEntry& entry) -> void;

}
