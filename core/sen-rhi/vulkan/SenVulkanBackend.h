#pragma once
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "sen-rhi/SenCommandBuffer.h"
#include "sen-rhi/SenTypes.h"
#include "umbrellas/access-modifiers.hpp"


struct ISlangBlob;
namespace Slang { template <typename T> class ComPtr; }

// ─── resource entries ─────────────────────────────────────────────────────────

struct SenVulkanTextureEntry {
};

struct SenVulkanBufferEntry {
};

struct SenVulkanSamplerEntry {
};

struct SenVulkanShaderEntry {
};


struct SenVulkanPipelineEntry {
};

struct SenVulkanSwapchainEntry {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    SenFormat format = SenFormat::Unknown;
    uint32_t width = 0;
    uint32_t height = 0;
};

// ─── backend ──────────────────────────────────────────────────────────────────

class SenVulkanBackend {
    expose
    static auto Init     (const SenDeviceDesc& desc) -> void;
    static auto Shutdown () -> void;

    
    expose // swapchain lifecycle
    static auto CreateSwapchain  (const SenSwapchainDesc& desc) -> SenSwapchain;
    static auto DestroySwapchain (SenSwapchain handle) -> void;
    static auto ResizeSwapchain  (SenSwapchain handle, uint32_t width, uint32_t height) -> void;
    static auto BeginFrame       (SenSwapchain handle) -> SenTexture;
    static auto EndFrame         (SenSwapchain handle) -> void;

    expose // command buffer factory
    static auto CreateCommandBuffer () -> SenCommandBuffer;

    expose // native API escape hatches (for ImGui, etc.)
    static auto GetNativeDevice  () -> void*;
    static auto GetNativeContext () -> void*;

    expose // debug annotation helpers
    static auto BeginDebugEvent (const std::string& label) -> void;
    static auto EndDebugEvent   () -> void;
    
    expose // textures
    static auto CreateTexture  (const SenTextureDesc& desc) -> SenTexture;
    static auto DestroyTexture (SenTexture handle) -> void;
    static auto LookupTexture  (SenTexture handle) -> SenVulkanTextureEntry&;

    expose // buffers
    static auto CreateBuffer  (const SenBufferDesc& desc) -> SenBuffer;
    static auto DestroyBuffer (SenBuffer handle) -> void;
    static auto LookupBuffer  (SenBuffer handle) -> SenVulkanBufferEntry&;
    static auto WriteBuffer   (SenBuffer handle, const void* data, uint32_t size) -> void;

    expose // samplers
    static auto CreateSampler  (const SenSamplerDesc& desc) -> SenSampler;
    static auto DestroySampler (SenSampler handle) -> void;
    static auto LookupSampler  (SenSampler handle) -> SenVulkanSamplerEntry&;
    
    expose // bind group layouts
    static auto CreateBindGroupLayout  (const SenBindGroupLayoutDesc& desc) -> SenBindGroupLayout;
    static auto DestroyBindGroupLayout (SenBindGroupLayout handle) -> void;
    static auto LookupBindGroupLayout  (SenBindGroupLayout handle) -> SenBindGroupLayoutDesc&;

    expose // bind groups
    static auto CreateBindGroup  (const SenBindGroupDesc& desc) -> SenBindGroup;
    static auto DestroyBindGroup (SenBindGroup handle) -> void;
    static auto LookupBindGroup  (SenBindGroup handle) -> SenBindGroupDesc&;

    
    expose // shaders
    static auto CreateShader (const SenShaderSourceDesc& sourceDesc) -> SenShader;
    static auto DestroyShader (SenShader handle) -> void;
    static auto LookupShader  (SenShader handle) -> SenVulkanShaderEntry&;

    expose // pipelines
    static auto CreatePipeline (const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline (SenPipeline handle) -> void;
    static auto LookupPipeline  (SenPipeline handle) -> SenVulkanPipelineEntry&;

    
    hide
    static VkInstance _instance;
    static VkPhysicalDevice _physicalDevice;
    static VkDevice _device;
    static VkQueue _queue;
    static uint32_t _queueFamilyIndex;
    static VkCommandPool _commandPool;
    
    static std::unordered_map<uint32_t, SenVulkanSwapchainEntry> _swapchains;
    static uint32_t _nextSwapchainId;
    
    static std::unordered_map<uint32_t, SenVulkanTextureEntry> _textures;
    static uint32_t _nextTextureId;

    static std::unordered_map<uint32_t, SenVulkanBufferEntry> _buffers;
    static uint32_t _nextBufferId;

    static std::unordered_map<uint32_t, SenVulkanSamplerEntry> _samplers;
    static uint32_t _nextSamplerId;

    static std::unordered_map<uint32_t, SenBindGroupLayoutDesc> _bindGroupLayouts;
    static uint32_t _nextBindGroupLayoutId;

    static std::unordered_map<uint32_t, SenBindGroupDesc> _bindGroups;
    static uint32_t _nextBindGroupId;

    static std::unordered_map<uint32_t, SenVulkanShaderEntry> _shaders;
    static uint32_t _nextShaderId;

    static std::unordered_map<uint32_t, SenVulkanPipelineEntry> _pipelines;
    static uint32_t _nextPipelineId;

};
