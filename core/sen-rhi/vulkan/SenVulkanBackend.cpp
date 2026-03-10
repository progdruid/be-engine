#include "SenVulkanBackend.h"
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "umbrellas/include-libassert.h"

// ─── static members ────────────────────────────────────────────────────────────────
VkInstance SenVulkanBackend::_instance;
VkPhysicalDevice SenVulkanBackend::_physicalDevice;
VkDevice SenVulkanBackend::_device;
VkQueue SenVulkanBackend::_queue;
uint32_t SenVulkanBackend::_queueFamilyIndex;
VkCommandPool SenVulkanBackend::_commandPool;

std::unordered_map<uint32_t, SenVulkanTextureEntry> SenVulkanBackend::_textures;
uint32_t SenVulkanBackend::_nextTextureId = 1;

std::unordered_map<uint32_t, SenVulkanBufferEntry> SenVulkanBackend::_buffers;
uint32_t SenVulkanBackend::_nextBufferId = 1;

std::unordered_map<uint32_t, SenVulkanSamplerEntry> SenVulkanBackend::_samplers;
uint32_t SenVulkanBackend::_nextSamplerId = 1;

std::unordered_map<uint32_t, SenVulkanShaderEntry> SenVulkanBackend::_shaders;
uint32_t SenVulkanBackend::_nextShaderId = 1;

std::unordered_map<uint32_t, SenVulkanPipelineEntry> SenVulkanBackend::_pipelines;
uint32_t SenVulkanBackend::_nextPipelineId = 1;

std::unordered_map<uint32_t, SenVulkanSwapchainEntry> SenVulkanBackend::_swapchains;
uint32_t SenVulkanBackend::_nextSwapchainId = 1;

std::unordered_map<uint32_t, SenBindGroupLayoutDesc> SenVulkanBackend::_bindGroupLayouts;
uint32_t SenVulkanBackend::_nextBindGroupLayoutId = 1;

std::unordered_map<uint32_t, SenBindGroupDesc> SenVulkanBackend::_bindGroups;
uint32_t SenVulkanBackend::_nextBindGroupId = 1;


// ─── backend ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::Init(const SenDeviceDesc& desc) -> void {
    // instance
    VkApplicationInfo appInfo {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "be-vulkan-application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "be-vulkan-engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo createInfo {
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
    be_assert(result == VK_SUCCESS, "Vulkan Failed to create instance!");
    
    
    // physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    _physicalDevice = devices[0];  // Pick first for now

    // graphics queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _queueFamilyIndex = i;
            break;
        }
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = _queueFamilyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority,
    };

    // logical device
    VkPhysicalDeviceFeatures deviceFeatures {};
    VkDeviceCreateInfo deviceCreateInfo {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos    = &queueCreateInfo,
        .pEnabledFeatures     = &deviceFeatures,
    };

    result = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device);
    be_assert(result == VK_SUCCESS, "Vulkan: Failed to create device!");
    vkGetDeviceQueue(_device, _queueFamilyIndex, 0, &_queue);
    
    
    // command pool
    VkCommandPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _queueFamilyIndex,
    };
    
    result = vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool);
    be_assert(result == VK_SUCCESS, "Failed to create command pool!");
    
}

auto SenVulkanBackend::Shutdown() -> void {
    // Destroy all swapchains first (they depend on device)
    std::vector<uint32_t> swapchainIds;
    for (const auto& pair : _swapchains) {
        swapchainIds.push_back(pair.first);
    }
    for (uint32_t id : swapchainIds) {
        DestroySwapchain(SenSwapchain { id });
    }

    if (_commandPool)   { vkDestroyCommandPool(_device, _commandPool, nullptr); _commandPool = VK_NULL_HANDLE; }
    if (_device)        { vkDestroyDevice(_device, nullptr);                    _device = VK_NULL_HANDLE; }
    if (_instance)      { vkDestroyInstance(_instance, nullptr);                _instance = VK_NULL_HANDLE; }
}


// ─── swapchain ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateSwapchain(const SenSwapchainDesc& desc) -> SenSwapchain {
    SenVulkanSwapchainEntry entry {};

    // 1. Create surface
    VkWin32SurfaceCreateInfoKHR surfaceInfo {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd      = (HWND)desc.NativeWindowHandle,
    };

    VkResult result = vkCreateWin32SurfaceKHR(_instance, &surfaceInfo, nullptr, &entry.surface);
    be_assert(result == VK_SUCCESS, "Failed to create surface!");

    // 2. Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, entry.surface, &capabilities);

    // Query supported formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, entry.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, entry.surface, &formatCount, surfaceFormats.data());

    // Choose format (prefer SRGB if available)
    VkSurfaceFormatKHR chosenFormat = surfaceFormats[0];
    for (const auto& format : surfaceFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = format;
            break;
        }
    }

    // Query supported present modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, entry.surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, entry.surface, &presentModeCount, presentModes.data());

    // Choose present mode
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;  // FIFO is always available
    for (const auto& mode : presentModes) {
        if (desc.PresentMode == SenPresentMode::Immediate && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            chosenPresentMode = mode;
            break;
        } else if (desc.PresentMode == SenPresentMode::Mailbox && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenPresentMode = mode;
            break;
        }
    }

    // 3. Create swapchain
    VkSwapchainCreateInfoKHR swapchainInfo {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = entry.surface,
        .minImageCount    = desc.BufferCount,
        .imageFormat      = chosenFormat.format,
        .imageColorSpace  = chosenFormat.colorSpace,
        .imageExtent      = {desc.Width, desc.Height},
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = chosenPresentMode,
        .clipped          = VK_TRUE,
    };

    result = vkCreateSwapchainKHR(_device, &swapchainInfo, nullptr, &entry.swapchain);
    be_assert(result == VK_SUCCESS, "Failed to create swapchain!");

    // 4. Get swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(_device, entry.swapchain, &imageCount, nullptr);
    entry.images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, entry.swapchain, &imageCount, entry.images.data());

    // 5. Create image views
    entry.imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo imageViewInfo {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image      = entry.images[i],
            .viewType   = VK_IMAGE_VIEW_TYPE_2D,
            .format     = chosenFormat.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        result = vkCreateImageView(_device, &imageViewInfo, nullptr, &entry.imageViews[i]);
        be_assert(result == VK_SUCCESS, "Failed to create image view!");
    }

    entry.format = desc.Format;
    entry.width = desc.Width;
    entry.height = desc.Height;

    const SenSwapchain handle { _nextSwapchainId++ };
    _swapchains[handle.ID] = entry;
    return handle;
}

auto SenVulkanBackend::DestroySwapchain(SenSwapchain handle) -> void {
    auto it = _swapchains.find(handle.ID);
    if (it != _swapchains.end()) {
        auto& entry = it->second;
        for (auto imageView : entry.imageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
        }
        if (entry.swapchain) {
            vkDestroySwapchainKHR(_device, entry.swapchain, nullptr);
        }
        if (entry.surface) {
            vkDestroySurfaceKHR(_instance, entry.surface, nullptr);
        }
        _swapchains.erase(it);
    }
}

auto SenVulkanBackend::ResizeSwapchain(SenSwapchain handle, uint32_t width, uint32_t height) -> void {
}

auto SenVulkanBackend::BeginFrame(SenSwapchain handle) -> SenTexture {
    return {};
}

auto SenVulkanBackend::EndFrame(SenSwapchain handle) -> void {
}


// ─── command buffer ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateCommandBuffer() -> SenCommandBuffer {
    return {};
}


// ─── native escape hatches ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::GetNativeDevice() -> void* {
    return nullptr;
}

auto SenVulkanBackend::GetNativeContext() -> void* {
    return nullptr;
}


// ─── debug ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::BeginDebugEvent(const std::string& label) -> void {
}

auto SenVulkanBackend::EndDebugEvent() -> void {
}


// ─── textures ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateTexture(const SenTextureDesc& desc) -> SenTexture {
    const SenTexture handle { _nextTextureId++ };
    _textures[handle.ID] = {};
    return handle;
}

auto SenVulkanBackend::DestroyTexture(SenTexture handle) -> void {
    _textures.erase(handle.ID);
}

auto SenVulkanBackend::LookupTexture(SenTexture handle) -> SenVulkanTextureEntry& {
    return _textures.at(handle.ID);
}


// ─── buffers ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateBuffer(const SenBufferDesc& desc) -> SenBuffer {
    const SenBuffer handle { _nextBufferId++ };
    _buffers[handle.ID] = {};
    return handle;
}

auto SenVulkanBackend::DestroyBuffer(SenBuffer handle) -> void {
    _buffers.erase(handle.ID);
}

auto SenVulkanBackend::LookupBuffer(SenBuffer handle) -> SenVulkanBufferEntry& {
    return _buffers.at(handle.ID);
}

auto SenVulkanBackend::WriteBuffer(SenBuffer handle, const void* data, uint32_t size) -> void {
}


// ─── samplers ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateSampler(const SenSamplerDesc& desc) -> SenSampler {
    const SenSampler handle { _nextSamplerId++ };
    _samplers[handle.ID] = {};
    return handle;
}

auto SenVulkanBackend::DestroySampler(SenSampler handle) -> void {
    _samplers.erase(handle.ID);
}

auto SenVulkanBackend::LookupSampler(SenSampler handle) -> SenVulkanSamplerEntry& {
    return _samplers.at(handle.ID);
}


// ─── bind group layouts ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateBindGroupLayout(const SenBindGroupLayoutDesc& desc) -> SenBindGroupLayout {
    const SenBindGroupLayout handle { _nextBindGroupLayoutId++ };
    _bindGroupLayouts[handle.ID] = desc;
    return handle;
}

auto SenVulkanBackend::DestroyBindGroupLayout(SenBindGroupLayout handle) -> void {
    _bindGroupLayouts.erase(handle.ID);
}

auto SenVulkanBackend::LookupBindGroupLayout(SenBindGroupLayout handle) -> SenBindGroupLayoutDesc& {
    return _bindGroupLayouts.at(handle.ID);
}


// ─── bind groups ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateBindGroup(const SenBindGroupDesc& desc) -> SenBindGroup {
    const SenBindGroup handle { _nextBindGroupId++ };
    _bindGroups[handle.ID] = desc;
    return handle;
}

auto SenVulkanBackend::DestroyBindGroup(SenBindGroup handle) -> void {
    _bindGroups.erase(handle.ID);
}

auto SenVulkanBackend::LookupBindGroup(SenBindGroup handle) -> SenBindGroupDesc& {
    return _bindGroups.at(handle.ID);
}


// ─── shaders ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    const SenShader handle { _nextShaderId++ };
    _shaders[handle.ID] = {};
    return handle;
}

auto SenVulkanBackend::DestroyShader(SenShader handle) -> void {
    _shaders.erase(handle.ID);
}

auto SenVulkanBackend::LookupShader(SenShader handle) -> SenVulkanShaderEntry& {
    return _shaders.at(handle.ID);
}


// ─── pipelines ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline {
    const SenPipeline handle { _nextPipelineId++ };
    _pipelines[handle.ID] = {};
    return handle;
}

auto SenVulkanBackend::DestroyPipeline(SenPipeline handle) -> void {
    _pipelines.erase(handle.ID);
}

auto SenVulkanBackend::LookupPipeline(SenPipeline handle) -> SenVulkanPipelineEntry& {
    return _pipelines.at(handle.ID);
}
