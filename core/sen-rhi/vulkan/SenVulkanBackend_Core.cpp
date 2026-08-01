#include "SenVulkanBackend.h"

#include <umbrellas/include-glfw.h>  // glfwGetRequiredInstanceExtensions
#include <sen-rhi/vulkan/SenVulkanValidation.h>
#include <sen-rhi/SenShaderCompiler.h>

#define VMA_IMPLEMENTATION
#include <ranges>
#include <vma/vk_mem_alloc.h>

#include <umbrellas/include-libassert.h>

// ─── static members ────────────────────────────────────────────────────────────────
VkInstance SenVulkanBackend::_instance;
VkPhysicalDevice SenVulkanBackend::_physicalDevice;
VkDevice SenVulkanBackend::_device;
VkQueue SenVulkanBackend::_queue;
uint32_t SenVulkanBackend::_queueFamilyIndex;
uint32_t SenVulkanBackend::_minUniformBufferOffsetAlignment = 256;
VkDescriptorPool SenVulkanBackend::_descriptorPool = VK_NULL_HANDLE;
VkCommandPool SenVulkanBackend::_commandPool;
VmaAllocator SenVulkanBackend::_allocator;

std::unordered_map<uint32_t, SenVulkanTextureEntry> SenVulkanBackend::_textures;     uint32_t SenVulkanBackend::_nextTextureId = 1;
std::unordered_map<uint32_t, SenVulkanBufferEntry> SenVulkanBackend::_buffers;       uint32_t SenVulkanBackend::_nextBufferId = 1;
std::unordered_map<uint32_t, SenVulkanSamplerEntry> SenVulkanBackend::_samplers;     uint32_t SenVulkanBackend::_nextSamplerId = 1;
std::unordered_map<uint32_t, SenVulkanShaderEntry> SenVulkanBackend::_shaders;       uint32_t SenVulkanBackend::_nextShaderId = 1;
std::unordered_map<uint32_t, SenVulkanPipelineEntry> SenVulkanBackend::_pipelines;   uint32_t SenVulkanBackend::_nextPipelineId = 1;
std::unordered_map<uint32_t, SenVulkanSwapchainEntry> SenVulkanBackend::_swapchains; uint32_t SenVulkanBackend::_nextSwapchainId = 1;
std::unordered_map<uint32_t, SenVulkanBindGroupEntry> SenVulkanBackend::_bindGroups; uint32_t SenVulkanBackend::_nextBindGroupId = 1;

// ─── device lifecycle ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::Init(const SenDeviceDesc& desc) -> void {
    SenShaderCompiler::Launch();

    // instance
    VkApplicationInfo appInfo {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "be-vulkan-application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "be-vulkan-engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount); // TODO: see above
    std::vector<const char*> instanceExtensions(glfwExts, glfwExts + glfwExtCount);
    std::vector<const char*> instanceLayers;
    const void* instancePNext = SenVulkanValidation::ConfigureForInstance(instanceLayers, instanceExtensions);

    VkInstanceCreateInfo createInfo {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = instancePNext,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = uint32_t(instanceLayers.size()),
        .ppEnabledLayerNames     = instanceLayers.data(),
        .enabledExtensionCount   = uint32_t(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
    be_assert(result == VK_SUCCESS, "Vulkan Failed to create instance!");
    SenVulkanValidation::CreateMessenger(_instance);


    // physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
    _physicalDevice = devices[0];  // Pick first for now

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
    _minUniformBufferOffsetAlignment = uint32_t(deviceProperties.limits.minUniformBufferOffsetAlignment);

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
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // swapchain is never core; dynamic rendering + sync2 are core in 1.3
    };

    // 1.0 features
    VkPhysicalDeviceFeatures enabled10Features {
        .tessellationShader = VK_TRUE,  // required by hull/domain stages (patch-list topologies)
        .depthClamp         = VK_TRUE,  // required by rasterizer depthClampEnable
        .samplerAnisotropy  = VK_TRUE,  // required by anisotropic samplers
    };
    // 1.1 core features
    VkPhysicalDeviceVulkan11Features enabled11Features {
        .sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE,  // Slang-emitted SPIR-V declares the DrawParameters capability
    };
    // 1.3 core features
    VkPhysicalDeviceVulkan13Features enabled13Features {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &enabled11Features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };
    VkDeviceCreateInfo deviceCreateInfo {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &enabled13Features,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueCreateInfo,
        .enabledExtensionCount   = uint32_t(std::size(deviceExtensions)),
        .ppEnabledExtensionNames = deviceExtensions,
        .pEnabledFeatures        = &enabled10Features,
    };

    result = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device);
    be_assert(result == VK_SUCCESS, "Vulkan: Failed to create device!");
    vkGetDeviceQueue(_device, _queueFamilyIndex, 0, &_queue);


    // command pool
    VkCommandPoolCreateInfo cmdPoolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _queueFamilyIndex,
    };

    result = vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &_commandPool);
    be_assert(result == VK_SUCCESS, "Failed to create command pool!");

    // VMA allocator
    VmaAllocatorCreateInfo allocatorInfo {
        .physicalDevice = _physicalDevice,
        .device         = _device,
        .instance       = _instance,
    };
    result = vmaCreateAllocator(&allocatorInfo, &_allocator);
    be_assert(result == VK_SUCCESS, "Failed to create VMA allocator!");

    // A material needs one descriptor set per arena chain it has landed in, so counts scale
    // with FramesInFlight rather than with the material count alone.
    std::array<VkDescriptorPoolSize, 6> poolSizes {
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          16384 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER,                16384 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          4096 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16384 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,           1024 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          1024 },
    };
    VkDescriptorPoolCreateInfo descPoolInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 16384,
        .poolSizeCount = uint32_t(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    };
    result = vkCreateDescriptorPool(_device, &descPoolInfo, nullptr, &_descriptorPool);
    be_assert(result == VK_SUCCESS, "Failed to create descriptor pool!");
}

auto SenVulkanBackend::Shutdown() -> void {
    // Destroy all swapchains first (they depend on device)
    auto swapchains = _swapchains;
    for (const auto id : swapchains | std::views::keys) {
        DestroySwapchain(SenSwapchain { id });
    }

    auto textures = _textures;
    for (const auto& id : textures | std::views::keys) {
        DestroyTexture(SenTexture { id });
    }

    auto buffers = _buffers;
    for (const auto& id : buffers | std::views::keys) {
        DestroyBuffer(SenBuffer { id });
    }

    auto pipelines = _pipelines;
    for (const auto& id : pipelines | std::views::keys) {
        DestroyPipeline(SenPipeline { id });
    }

    auto shaders = _shaders;
    for (const auto& id : shaders | std::views::keys) {
        DestroyShader(SenShader { id });
    }

    auto bindGroups = _bindGroups;
    for (const auto& id : bindGroups | std::views::keys) {
        DestroyBindGroup(SenBindGroup { id });
    }

    auto samplers = _samplers;
    for (const auto& id : samplers | std::views::keys) {
        DestroySampler(SenSampler { id });
    }

    if (_commandPool)      { vkDestroyCommandPool(_device, _commandPool, nullptr); _commandPool = VK_NULL_HANDLE; }
    if (_descriptorPool)   { vkDestroyDescriptorPool(_device, _descriptorPool, nullptr); _descriptorPool = VK_NULL_HANDLE; }
    if (_allocator)        { vmaDestroyAllocator(_allocator); _allocator = VK_NULL_HANDLE; }
    if (_device)           { vkDestroyDevice(_device, nullptr); _device = VK_NULL_HANDLE; }
    SenVulkanValidation::DestroyMessenger(_instance);
    if (_instance)         { vkDestroyInstance(_instance, nullptr); _instance = VK_NULL_HANDLE; }
}

auto SenVulkanBackend::WaitIdle() -> void {
    vkDeviceWaitIdle(_device);
}

// ─── command buffer ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::AllocateCommandBuffer() -> SenVulkanCommandBuffer {
    VkCommandBufferAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult result = vkAllocateCommandBuffers(_device, &allocInfo, &cmd);
    be_assert(result == VK_SUCCESS, "Failed to allocate command buffer!");
    return SenVulkanCommandBuffer(cmd);
}

auto SenVulkanBackend::SubmitImmediate(SenVulkanCommandBuffer& cmd) -> void {
    const VkCommandBuffer vkCmd = cmd.GetNativeHandle();

    const VkSubmitInfo submitInfo {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &vkCmd,
    };

    const VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(_device, &fenceInfo, nullptr, &fence);

    vkQueueSubmit(_queue, 1, &submitInfo, fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(_device, fence, nullptr);
}

// ─── native escape hatches ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::GetNativeDevice() -> void* {
    return _device;
}

auto SenVulkanBackend::GetNativeInstance() -> void* {
    return _instance;
}

auto SenVulkanBackend::GetNativePhysicalDevice() -> void* {
    return _physicalDevice;
}

auto SenVulkanBackend::GetNativeQueue() -> void* {
    return _queue;
}

auto SenVulkanBackend::GetNativeQueueFamilyIndex() -> uint32_t {
    return _queueFamilyIndex;
}

// ─── debug ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::BeginDebugEvent(const std::string& label) -> void {
}

auto SenVulkanBackend::EndDebugEvent() -> void {
}
