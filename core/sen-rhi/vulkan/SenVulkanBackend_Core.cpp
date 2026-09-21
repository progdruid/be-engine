#include "SenVulkanBackend.h"

#include <umbrellas/include-glfw.h>  // glfwGetRequiredInstanceExtensions
#include <sen-rhi/vulkan/SenVulkanValidation.h>

#define VMA_IMPLEMENTATION
#include <cstdio>
#include <ranges>
#include <vma/vk_mem_alloc.h>

#include <umbrellas/include-libassert.h>

// ─── static members ────────────────────────────────────────────────────────────────
VkInstance SenVulkanBackend::_instance;
VkPhysicalDevice SenVulkanBackend::_physicalDevice;
VkDevice SenVulkanBackend::_device;
VkQueue SenVulkanBackend::_queue;
uint32_t SenVulkanBackend::_queueFamilyIndex;
VkSemaphore SenVulkanBackend::_timeline = VK_NULL_HANDLE;
uint64_t SenVulkanBackend::_timelineValue = 0;
VkCommandPool SenVulkanBackend::_commandPool;
VmaAllocator SenVulkanBackend::_allocator;

SenSlotMap<SenVulkanTextureEntry, SenTexture> SenVulkanBackend::_textures;
SenSlotMap<SenVulkanViewEntry, SenView> SenVulkanBackend::_views;
SenSlotMap<SenVulkanBufferEntry, SenBuffer> SenVulkanBackend::_buffers;
SenSlotMap<SenVulkanSamplerEntry, SenSampler> SenVulkanBackend::_samplers;
SenSlotMap<SenVulkanPipelineEntry, SenPipeline> SenVulkanBackend::_pipelines;
SenSlotMap<SenVulkanSwapchainEntry, SenSwapchain> SenVulkanBackend::_swapchains;

VkDescriptorSetLayout SenVulkanBackend::_bindlessLayout = VK_NULL_HANDLE;
VkDescriptorPool      SenVulkanBackend::_bindlessPool   = VK_NULL_HANDLE;
VkDescriptorSet       SenVulkanBackend::_bindlessSet    = VK_NULL_HANDLE;

// ─── device lifecycle ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::Init(const SenDeviceDesc& desc) -> void {
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
    be_assert(deviceCount > 0, "Vulkan: no physical devices found");

    _physicalDevice = VK_NULL_HANDLE;
    int bestRank = -1;
    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        int rank = 0;
        const char* type = "other";
        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: rank = 4; type = "discrete"; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: rank = 3; type = "integrated"; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: rank = 2; type = "virtual"; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: rank = 1; type = "cpu"; break;
            default: break;
        }
        std::fprintf(stderr, "[vulkan] device %u: %s (%s)\n", i, props.deviceName, type);

        if (rank > bestRank) {
            bestRank = rank;
            _physicalDevice = devices[i];
        }
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
    std::fprintf(stderr, "[vulkan] selected: %s\n", deviceProperties.deviceName);

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
        .imageCubeArray     = VK_TRUE,  // required by cube-array views (point-light shadow arrays)
        .tessellationShader = VK_TRUE,  // required by hull/domain stages (patch-list topologies)
        .depthClamp         = VK_TRUE,  // required by rasterizer depthClampEnable
        .samplerAnisotropy  = VK_TRUE,  // required by anisotropic samplers
    };
    // 1.1 core features
    VkPhysicalDeviceVulkan11Features enabled11Features {
        .sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE,  // Slang-emitted SPIR-V declares the DrawParameters capability
    };
    // 1.2 core features
    VkPhysicalDeviceVulkan12Features enabled12Features {
        .sType                                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext                                        = &enabled11Features,
        .descriptorIndexing                           = VK_TRUE,  // bindless: descriptor-indexing umbrella
        .shaderSampledImageArrayNonUniformIndexing    = VK_TRUE,  // bindless: index sampled-image heap by non-uniform id
        .shaderStorageImageArrayNonUniformIndexing    = VK_TRUE,  // bindless: index storage-image (RWTexture2D) heap
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,  // bindless: write heap slots after the set is bound
        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,  // bindless: same for storage-image heap
        .descriptorBindingPartiallyBound              = VK_TRUE,  // bindless: unwritten heap slots need not be valid
        .descriptorBindingVariableDescriptorCount     = VK_TRUE,  // bindless: heap sized at set-alloc time, not layout time
        .runtimeDescriptorArray                       = VK_TRUE,  // bindless: unbounded Texture2D[] etc. in shaders
        .scalarBlockLayout                            = VK_TRUE,  // BDA: scalar (natural) layout for pointer-backed cbuffers
        .timelineSemaphore                            = VK_TRUE,
        .bufferDeviceAddress                          = VK_TRUE,  // BDA: buffers as GpuAddress pointers
    };
    // 1.3 core features
    VkPhysicalDeviceVulkan13Features enabled13Features {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &enabled12Features,
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

    // timeline semaphore
    VkSemaphoreTypeCreateInfo timelineType {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0,
    };
    VkSemaphoreCreateInfo timelineInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timelineType,
    };
    result = vkCreateSemaphore(_device, &timelineInfo, nullptr, &_timeline);
    be_assert(result == VK_SUCCESS, "Failed to create timeline semaphore!");
    _timelineValue = 0;

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
        .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = _physicalDevice,
        .device         = _device,
        .instance       = _instance,
    };
    result = vmaCreateAllocator(&allocatorInfo, &_allocator);
    be_assert(result == VK_SUCCESS, "Failed to create VMA allocator!");

    InitBindless();
}

auto SenVulkanBackend::Shutdown() -> void {
    WaitIdle();

    // Destroy all swapchains first (they depend on device)
    for (const auto handle : _swapchains.GetLiveHandles()) { DestroySwapchain(handle); }
    for (const auto handle : _views.GetLiveHandles())      { DestroyView(handle); }
    for (const auto handle : _textures.GetLiveHandles())   { DestroyTexture(handle); }
    for (const auto handle : _buffers.GetLiveHandles())    { DestroyBuffer(handle); }
    for (const auto handle : _pipelines.GetLiveHandles())  { DestroyPipeline(handle); }
    for (const auto handle : _samplers.GetLiveHandles())   { DestroySampler(handle); }

    ShutdownBindless();
    if (_timeline)         { vkDestroySemaphore(_device, _timeline, nullptr); _timeline = VK_NULL_HANDLE; }
    if (_commandPool)      { vkDestroyCommandPool(_device, _commandPool, nullptr); _commandPool = VK_NULL_HANDLE; }
    if (_allocator)        { vmaDestroyAllocator(_allocator); _allocator = VK_NULL_HANDLE; }
    if (_device)           { vkDestroyDevice(_device, nullptr); _device = VK_NULL_HANDLE; }
    SenVulkanValidation::DestroyMessenger(_instance);
    if (_instance)         { vkDestroyInstance(_instance, nullptr); _instance = VK_NULL_HANDLE; }
}

auto SenVulkanBackend::WaitIdle() -> void {
    vkDeviceWaitIdle(_device);
}

auto SenVulkanBackend::GetCaps() -> SenCaps {
    VkPhysicalDeviceProperties props {};
    vkGetPhysicalDeviceProperties(_physicalDevice, &props);

    SenCaps caps {
        .TextureSlots = BindlessTextureSlots,
        .StorageSlots = BindlessStorageSlots,
        .SamplerSlots = BindlessSamplerSlots,
    };
    std::snprintf(caps.DeviceName, sizeof(caps.DeviceName), "%s", props.deviceName);
    return caps;
}

auto SenVulkanBackend::IsSubmissionComplete(SenSubmission submission) -> bool {
    uint64_t completedValue = 0;
    vkGetSemaphoreCounterValue(_device, _timeline, &completedValue);
    return completedValue >= submission.Value;
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

auto SenVulkanBackend::SubmitImmediate(SenVulkanCommandBuffer& cmd) -> SenSubmission {
    const VkCommandBufferSubmitInfo cmdInfo {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd.GetNativeHandle(),
    };
    
    const uint64_t signalValue = ++_timelineValue;
    const VkSemaphoreSubmitInfo signalInfo {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = _timeline,
        .value     = signalValue,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };
    
    const VkSubmitInfo2 submitInfo {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &cmdInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signalInfo,
    };
    vkQueueSubmit2(_queue, 1, &submitInfo, VK_NULL_HANDLE);
    
    const VkSemaphoreWaitInfo waitInfo {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &_timeline,
        .pValues        = &signalValue,
    };
    vkWaitSemaphores(_device, &waitInfo, UINT64_MAX);

    return SenSubmission { signalValue };
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
