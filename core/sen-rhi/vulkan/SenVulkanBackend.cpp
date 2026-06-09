#include "SenVulkanBackend.h"
#include <vulkan/vulkan_core.h>
// TODO: surface creation currently delegates to GLFW, which ties sen-rhi to a windowing library.
// The correct fix is a small platform-dispatch service inside sen that, given (platform, display server, API choice),
// returns the appropriate surface + required instance extensions — with no windowing lib
// knowledge at the sen level. Until that service exists, GLFW is used here as a stopgap.
#include <umbrellas/include-glfw.h>  // vulkan_core.h already included above; GLFW will still expose glfwCreateWindowSurface via VK_VERSION_1_0
#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <sen-rhi/vulkan/SenVulkanValidation.h>
#include <sen-rhi/SenShaderCompiler.h>

#define VMA_IMPLEMENTATION
#include <ranges>
#include <cstring>
#include <vma/vk_mem_alloc.h>

#include <umbrellas/include-libassert.h>

// region ────────── static members ────────────────────────────────────────────────────────────────
VkInstance SenVulkanBackend::_instance;
VkPhysicalDevice SenVulkanBackend::_physicalDevice;
VkDevice SenVulkanBackend::_device;
VkQueue SenVulkanBackend::_queue;
uint32_t SenVulkanBackend::_queueFamilyIndex;
VkDescriptorPool SenVulkanBackend::_descriptorPool = VK_NULL_HANDLE;
VkCommandPool SenVulkanBackend::_commandPool;
VmaAllocator SenVulkanBackend::_allocator;

VkCommandBuffer            SenVulkanBackend::_activeCommandBuffer     = VK_NULL_HANDLE;
SenVulkanCommandBuffer     SenVulkanBackend::_commandBufferInstance   = {};

std::unordered_map<uint32_t, SenVulkanTextureEntry> SenVulkanBackend::_textures;     uint32_t SenVulkanBackend::_nextTextureId = 1;
std::unordered_map<uint32_t, SenVulkanBufferEntry> SenVulkanBackend::_buffers;       uint32_t SenVulkanBackend::_nextBufferId = 1;
std::unordered_map<uint32_t, SenVulkanSamplerEntry> SenVulkanBackend::_samplers;     uint32_t SenVulkanBackend::_nextSamplerId = 1;
std::unordered_map<uint32_t, SenVulkanShaderEntry> SenVulkanBackend::_shaders;       uint32_t SenVulkanBackend::_nextShaderId = 1;
std::unordered_map<uint32_t, SenVulkanPipelineEntry> SenVulkanBackend::_pipelines;   uint32_t SenVulkanBackend::_nextPipelineId = 1;
std::unordered_map<uint32_t, SenVulkanSwapchainEntry> SenVulkanBackend::_swapchains; uint32_t SenVulkanBackend::_nextSwapchainId = 1;
std::unordered_map<uint32_t, SenVulkanBindGroupEntry> SenVulkanBackend::_bindGroups; uint32_t SenVulkanBackend::_nextBindGroupId = 1;
//endregion

// region ────────── backend ────────────────────────────────────────────────────────────────
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
    const void* instancePNext = Sen::Vulkan::Validation::ConfigureInstance(instanceLayers, instanceExtensions);

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
    Sen::Vulkan::Validation::CreateMessenger(_instance);


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
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // swapchain is never core; dynamic rendering + sync2 are core in 1.3
    };

    // 1.0 features
    VkPhysicalDeviceFeatures enabled10Features {
        .depthClamp        = VK_TRUE,  // required by rasterizer depthClampEnable
        .samplerAnisotropy = VK_TRUE,  // required by anisotropic samplers
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

    std::array<VkDescriptorPoolSize, 3> poolSizes {
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER, 4096 },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 },
    };
    VkDescriptorPoolCreateInfo descPoolInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 4096,
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

    if (_commandPool)      { vkDestroyCommandPool(_device, _commandPool, nullptr); _commandPool = VK_NULL_HANDLE; }
    if (_descriptorPool)   { vkDestroyDescriptorPool(_device, _descriptorPool, nullptr); _descriptorPool = VK_NULL_HANDLE; }
    if (_allocator)        { vmaDestroyAllocator(_allocator); _allocator = VK_NULL_HANDLE; }
    if (_device)           { vkDestroyDevice(_device, nullptr); _device = VK_NULL_HANDLE; }
    Sen::Vulkan::Validation::DestroyMessenger(_instance);
    if (_instance)         { vkDestroyInstance(_instance, nullptr); _instance = VK_NULL_HANDLE; }
}
//endregion

// region ────────── swapchain ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateSwapchain(const SenSwapchainDesc& desc) -> SenSwapchain {
    SenVulkanSwapchainEntry entry {};

    // 1. Create surface — TODO: see above
    VkResult result = glfwCreateWindowSurface(_instance, static_cast<GLFWwindow*>(desc.NativeWindowHandle), nullptr, &entry.Surface);
    be_assert(result == VK_SUCCESS, "Failed to create surface!");

    // 2. Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, entry.Surface, &capabilities);

    // Query supported formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, entry.Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, entry.Surface, &formatCount, surfaceFormats.data());

    // Choose format: prefer BGRA8 or RGBA8 UNORM with SRGB_NONLINEAR color space
    VkSurfaceFormatKHR chosenFormat = surfaceFormats[0];
    for (const auto& format : surfaceFormats) {
        if (format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { continue; }
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM) {
            chosenFormat = format; 
            break;
        }
    }

    // Query supported present modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, entry.Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, entry.Surface, &presentModeCount, presentModes.data());

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
    uint32_t minImageCount = desc.BufferCount;
    if (minImageCount < capabilities.minImageCount) { minImageCount = capabilities.minImageCount; }
    if (capabilities.maxImageCount != 0 && minImageCount > capabilities.maxImageCount) { minImageCount = capabilities.maxImageCount; }

    // currentExtent == 0xFFFFFFFF means the surface lets us pick (clamp to bounds); otherwise it is mandatory.
    VkExtent2D imageExtent = capabilities.currentExtent;
    if (imageExtent.width == UINT32_MAX) {
        imageExtent = { desc.Width, desc.Height };
        if (imageExtent.width  < capabilities.minImageExtent.width)  { imageExtent.width  = capabilities.minImageExtent.width; }
        if (imageExtent.width  > capabilities.maxImageExtent.width)  { imageExtent.width  = capabilities.maxImageExtent.width; }
        if (imageExtent.height < capabilities.minImageExtent.height) { imageExtent.height = capabilities.minImageExtent.height; }
        if (imageExtent.height > capabilities.maxImageExtent.height) { imageExtent.height = capabilities.maxImageExtent.height; }
    }

    VkSwapchainCreateInfoKHR swapchainInfo {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = entry.Surface,
        .minImageCount    = minImageCount,
        .imageFormat      = chosenFormat.format,
        .imageColorSpace  = chosenFormat.colorSpace,
        .imageExtent      = imageExtent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = chosenPresentMode,
        .clipped          = VK_TRUE,
    };

    // std::fprintf(stderr,
    //     "[Swapchain] requested=%ux%u  currentExtent=%ux%u  min=%ux%u  max=%ux%u\n",
    //     desc.Width, desc.Height,
    //     capabilities.currentExtent.width, capabilities.currentExtent.height,
    //     capabilities.minImageExtent.width, capabilities.minImageExtent.height,
    //     capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);

    result = vkCreateSwapchainKHR(_device, &swapchainInfo, nullptr, &entry.Swapchain);
    be_assert(result == VK_SUCCESS, "Failed to create swapchain!");

    // 4. Get swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(_device, entry.Swapchain, &imageCount, nullptr);
    entry.Images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, entry.Swapchain, &imageCount, entry.Images.data());

    // 5. Create image views
    entry.ImageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo imageViewInfo {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image      = entry.Images[i],
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

        result = vkCreateImageView(_device, &imageViewInfo, nullptr, &entry.ImageViews[i]);
        be_assert(result == VK_SUCCESS, "Failed to create image view!");
    }

    entry.NativeWindowHandle = desc.NativeWindowHandle;
    entry.Width       = imageExtent.width;
    entry.Height      = imageExtent.height;
    entry.BufferCount = desc.BufferCount;
    entry.Format      = Sen::Vulkan::FromVkFormat(chosenFormat.format);
    entry.PresentMode = desc.PresentMode;

    // 6. Register each swapchain image as a SenTexture (MipRTVs[0] = image view)
    entry.Textures.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        SenVulkanTextureEntry texEntry {};
        texEntry.Image  = entry.Images[i];
        texEntry.Format = chosenFormat.format;
        texEntry.MipRTVs.push_back(entry.ImageViews[i]);

        const SenTexture texHandle { _nextTextureId++ };
        _textures[texHandle.ID] = texEntry;
        entry.Textures[i] = texHandle;
    }

    // 7. Create sync objects
    VkSemaphoreCreateInfo semaphoreInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,  // start signaled so first frame doesn't wait forever
    };
    result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.ImageAvailableSemaphore);
    be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
    result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.RenderFinishedSemaphore);
    be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
    result = vkCreateFence(_device, &fenceInfo, nullptr, &entry.InFlightFence);
    be_assert(result == VK_SUCCESS, "Failed to create fence!");

    const SenSwapchain handle { _nextSwapchainId++ };
    _swapchains[handle.ID] = std::move(entry);
    return handle;
}

auto SenVulkanBackend::DestroySwapchain(SenSwapchain handle) -> void {
    auto it = _swapchains.find(handle.ID);
    if (it != _swapchains.end()) {
        auto& entry = it->second;

        // Remove swapchain texture entries (image views are owned by swapchain, not VMA)
        for (const auto& tex : entry.Textures) {
            _textures.erase(tex.ID);
        }

        if (entry.InFlightFence)           { vkDestroyFence(_device, entry.InFlightFence, nullptr); }
        if (entry.ImageAvailableSemaphore) { vkDestroySemaphore(_device, entry.ImageAvailableSemaphore, nullptr); }
        if (entry.RenderFinishedSemaphore) { vkDestroySemaphore(_device, entry.RenderFinishedSemaphore, nullptr); }
        for (auto imageView : entry.ImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
        }
        if (entry.Swapchain) { vkDestroySwapchainKHR(_device, entry.Swapchain, nullptr); }
        if (entry.Surface)   { vkDestroySurfaceKHR(_instance, entry.Surface, nullptr); }

        _swapchains.erase(it);
    }
}

auto SenVulkanBackend::ResizeSwapchain(SenSwapchain& handle, uint32_t width, uint32_t height) -> void {
    auto entry = _swapchains[handle.ID];
    
    DestroySwapchain(handle);
    handle = CreateSwapchain({
        .NativeWindowHandle = entry.NativeWindowHandle,
        .Width = width,
        .Height = height,
        .BufferCount = entry.BufferCount,
        .Format = entry.Format,
        .PresentMode = entry.PresentMode,
    });
}

auto SenVulkanBackend::WaitIdle() -> void {
    vkDeviceWaitIdle(_device);
}

auto SenVulkanBackend::GetSwapchainFormat(SenSwapchain handle) -> SenFormat {
    return _swapchains.at(handle.ID).Format;
}

auto SenVulkanBackend::BeginFrame(SenSwapchain handle) -> SenTexture {
    auto& entry = _swapchains.at(handle.ID);

    // Wait for GPU to finish the previous frame
    vkWaitForFences(_device, 1, &entry.InFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(_device, 1, &entry.InFlightFence);

    // Acquire the next swapchain image
    vkAcquireNextImageKHR(
        _device, entry.Swapchain, UINT64_MAX,
        entry.ImageAvailableSemaphore, VK_NULL_HANDLE,
        &entry.CurrentImageIndex
    );

    // Reset and begin recording the active command buffer
    _commandBufferInstance.ResetPerFrameState();
    vkResetCommandBuffer(_activeCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(_activeCommandBuffer, &beginInfo);

    // Transition swapchain image UNDEFINED/PRESENT_SRC → COLOR_ATTACHMENT_OPTIMAL
    auto& texEntry = _textures.at(entry.Textures[entry.CurrentImageIndex].ID);
    TransitionImageLayout(
        _activeCommandBuffer, texEntry.Image, VK_IMAGE_ASPECT_COLOR_BIT,
        texEntry.CurrentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        1, 1
    );
    texEntry.CurrentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    return entry.Textures[entry.CurrentImageIndex];
}

auto SenVulkanBackend::EndFrame(SenSwapchain handle) -> void {
    auto& entry = _swapchains.at(handle.ID);

    // Transition swapchain image COLOR_ATTACHMENT_OPTIMAL → PRESENT_SRC_KHR
    auto& texEntry = _textures.at(entry.Textures[entry.CurrentImageIndex].ID);
    TransitionImageLayout(
        _activeCommandBuffer, texEntry.Image, VK_IMAGE_ASPECT_COLOR_BIT,
        texEntry.CurrentLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        1, 1
    );
    texEntry.CurrentLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkEndCommandBuffer(_activeCommandBuffer);

    // Submit: wait on imageAvailable, signal renderFinished, signal fence when done
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &entry.ImageAvailableSemaphore,
        .pWaitDstStageMask    = &waitStage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &_activeCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &entry.RenderFinishedSemaphore,
    };
    vkQueueSubmit(_queue, 1, &submitInfo, entry.InFlightFence);

    // Present: wait on renderFinished
    VkPresentInfoKHR presentInfo {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &entry.RenderFinishedSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &entry.Swapchain,
        .pImageIndices      = &entry.CurrentImageIndex,
    };
    vkQueuePresentKHR(_queue, &presentInfo);
}
//endregion

// ─── command buffer ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateCommandBuffer() -> void {
    be_assert(_activeCommandBuffer == VK_NULL_HANDLE, "CreateCommandBuffer called more than once");

    VkCommandBufferAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkResult result = vkAllocateCommandBuffers(_device, &allocInfo, &_activeCommandBuffer);
    be_assert(result == VK_SUCCESS, "Failed to allocate command buffer!");
    _commandBufferInstance = SenVulkanCommandBuffer(_activeCommandBuffer);
}

auto SenVulkanBackend::GetCommandBuffer() -> SenVulkanCommandBuffer& {
    return _commandBufferInstance;
}


// ─── native escape hatches ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::GetNativeDevice() -> void* {
    return _device;
}

auto SenVulkanBackend::GetNativeContext() -> void* {
    return _activeCommandBuffer;
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


// region ────────── textures ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateTexture(const SenTextureDesc& desc) -> SenTexture {
    const SenTexture handle { _nextTextureId++ };
    auto& entry = _textures[handle.ID];

    if (desc.Cubemap) {
        CreateTextureCubemap(desc, entry);
    } else {
        CreateTexture2D(desc, entry);
    }

    return handle;
}

auto SenVulkanBackend::DestroyTexture(SenTexture handle) -> void {
    auto it = _textures.find(handle.ID);
    if (it != _textures.end()) {
        auto& entry = it->second;
        auto destroy = [&](VkImageView v) -> void { if (v) { vkDestroyImageView(_device, v, nullptr); } };

        destroy(entry.SRV);
        destroy(entry.DSV);
        for (auto v : entry.MipRTVs) { destroy(v); }
        for (auto v : entry.CubemapDSVs) { destroy(v); }
        for (auto& mips : entry.CubemapMipRTVs) { for (auto v : mips) { destroy(v); } }

        vmaDestroyImage(_allocator, entry.Image, entry.Allocation);
        _textures.erase(it);
    }
}

auto SenVulkanBackend::LookupTexture(SenTexture handle) -> SenVulkanTextureEntry& {
    return _textures.at(handle.ID);
}



auto SenVulkanBackend::CreateTexture2D(const SenTextureDesc& desc, SenVulkanTextureEntry& entry) -> void {
    const VkFormat           format  = Sen::Vulkan::ToFormat(desc.Format);
    const VkImageUsageFlags  usage   = Sen::Vulkan::ToImageUsageFlags(desc.Usage, desc.Data != nullptr);
    const bool               isDepth = HasAny(desc.Usage, SenTextureUsage::DepthStencil);
    const VkImageAspectFlags aspect  = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    entry.Format = format;

    VkImageCreateInfo imageInfo {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { desc.Width, desc.Height, 1 },
        .mipLevels     = desc.Mips,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
    VkResult result = vmaCreateImage(_allocator, &imageInfo, &allocInfo, &entry.Image, &entry.Allocation, nullptr);
    be_assert(result == VK_SUCCESS, "Failed to create 2D image!");

    if (desc.Data) {
        const uint32_t dataSize = desc.Width * desc.Height * Sen::Vulkan::BytesPerPixel(desc.Format);
        UploadToDeviceImage(entry.Image, aspect, desc.Data, dataSize, desc.Width, desc.Height, desc.Mips, 1);
        entry.CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (HasAny(desc.Usage, SenTextureUsage::ShaderResource)) {
        entry.SRV = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, desc.Mips, 0, 1);
    }
    if (HasAny(desc.Usage, SenTextureUsage::DepthStencil)) {
        entry.DSV = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, 1, 0, 1);
    }
    if (HasAny(desc.Usage, SenTextureUsage::RenderTarget)) {
        entry.MipRTVs.resize(desc.Mips);
        for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
            entry.MipRTVs[mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, 0, 1);
        }
    }
}

auto SenVulkanBackend::CreateTextureCubemap(const SenTextureDesc& desc, SenVulkanTextureEntry& entry) -> void {
    const VkFormat           format  = Sen::Vulkan::ToFormat(desc.Format);
    const VkImageUsageFlags  usage   = Sen::Vulkan::ToImageUsageFlags(desc.Usage, desc.Data != nullptr);
    const bool               isDepth = HasAny(desc.Usage, SenTextureUsage::DepthStencil);
    const VkImageAspectFlags aspect  = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    entry.Format = format;

    VkImageCreateInfo imageInfo {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { desc.Width, desc.Height, 1 },
        .mipLevels     = desc.Mips,
        .arrayLayers   = 6,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
    VkResult result = vmaCreateImage(_allocator, &imageInfo, &allocInfo, &entry.Image, &entry.Allocation, nullptr);
    be_assert(result == VK_SUCCESS, "Failed to create cubemap image!");

    if (desc.Data) {
        const uint32_t faceSize = desc.Width * desc.Height * Sen::Vulkan::BytesPerPixel(desc.Format);
        UploadToDeviceImage(entry.Image, aspect, desc.Data, faceSize * 6, desc.Width, desc.Height, desc.Mips, 6);
        entry.CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (HasAny(desc.Usage, SenTextureUsage::ShaderResource)) {
        entry.SRV = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_CUBE, aspect, 0, desc.Mips, 0, 6);
    }
    if (HasAny(desc.Usage, SenTextureUsage::DepthStencil)) {
        for (uint32_t face = 0; face < 6; ++face) {
            entry.CubemapDSVs[face] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, 1, face, 1);
        }
    }
    if (HasAny(desc.Usage, SenTextureUsage::RenderTarget)) {
        for (uint32_t face = 0; face < 6; ++face) {
            entry.CubemapMipRTVs[face].resize(desc.Mips);
            for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
                entry.CubemapMipRTVs[face][mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, face, 1);
            }
        }
    }
}

auto SenVulkanBackend::UploadToDeviceImage(VkImage image, VkImageAspectFlags aspect, const void* data, uint32_t dataSize, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layerCount) -> void {
    VkBufferCreateInfo stagingInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = dataSize,
        .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VmaAllocationInfo stagingResult;
    VkResult result = vmaCreateBuffer(_allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, &stagingResult);
    be_assert(result == VK_SUCCESS, "Failed to create texture staging buffer!");
    memcpy(stagingResult.pMappedData, data, dataSize);

    VkCommandBufferAllocateInfo cmdAlloc {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(_device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    TransitionImageLayout(cmd, image, aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, layerCount);

    // One copy region per layer (face), data expected to be laid out sequentially face0, face1, ...
    std::vector<VkBufferImageCopy> regions(layerCount);
    const uint32_t faceSize = dataSize / layerCount;
    for (uint32_t layer = 0; layer < layerCount; ++layer) {
        regions[layer] = {
            .bufferOffset      = VkDeviceSize(layer * faceSize),
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource  = { aspect, 0, layer, 1 },
            .imageOffset       = { 0, 0, 0 },
            .imageExtent       = { width, height, 1 },
        };
    }
    vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, regions.data());

    TransitionImageLayout(cmd, image, aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, layerCount);

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(_device, &fenceInfo, nullptr, &fence);
    VkSubmitInfo submitInfo { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    vkQueueSubmit(_queue, 1, &submitInfo, fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(_device, fence, nullptr);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
    vmaDestroyBuffer(_allocator, stagingBuffer, stagingAlloc);
}

auto SenVulkanBackend::TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount) -> void {
    auto scopeFor = [](VkImageLayout layout, VkPipelineStageFlags2& stage, VkAccessFlags2& access) -> void {
        switch (layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
                access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                // Present is synchronised by the swapchain semaphore, not by this barrier.
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
                break;
            default:
                be_assert(false, "TransitionImageLayout: unsupported layout");
                stage  = VK_PIPELINE_STAGE_2_NONE;
                access = VK_ACCESS_2_NONE;
        }
    };

    VkPipelineStageFlags2 srcStage, dstStage;
    VkAccessFlags2        srcAccess, dstAccess;
    scopeFor(oldLayout, srcStage, srcAccess);
    scopeFor(newLayout, dstStage, dstAccess);

    VkImageMemoryBarrier2 barrier {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask        = srcStage,
        .srcAccessMask       = srcAccess,
        .dstStageMask        = dstStage,
        .dstAccessMask       = dstAccess,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = { aspect, 0, mipLevels, 0, layerCount },
    };

    VkDependencyInfo dependency {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier,
    };
    vkCmdPipelineBarrier2(cmd, &dependency);
}

auto SenVulkanBackend::CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipLevels, uint32_t baseLayer, uint32_t layerCount) -> VkImageView {
    VkImageViewCreateInfo viewInfo {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = image,
        .viewType = viewType,
        .format   = format,
        .subresourceRange = {
            .aspectMask     = aspect,
            .baseMipLevel   = baseMip,
            .levelCount     = mipLevels,
            .baseArrayLayer = baseLayer,
            .layerCount     = layerCount,
        },
    };
    VkImageView view;
    VkResult result = vkCreateImageView(_device, &viewInfo, nullptr, &view);
    be_assert(result == VK_SUCCESS, "Failed to create image view!");
    return view;
}
//endregion

// region ────────── buffers ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateBuffer(const SenBufferDesc& desc) -> SenBuffer {
    const SenBuffer handle { _nextBufferId++ };
    auto& entry = _buffers[handle.ID];
    entry.Access = desc.Access;
    entry.Size   = desc.Size;

    VkBufferUsageFlags usageFlags = 0;
    switch (desc.Usage) {
        case SenBufferUsage::Vertex:   usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;  break;
        case SenBufferUsage::Index:    usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;   break;
        case SenBufferUsage::Constant: usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
    }

    VkBufferCreateInfo bufferInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = desc.Size,
        .usage       = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (desc.Access == SenBufferAccess::Dynamic) {
        // Host-visible, persistently mapped — CPU writes via memcpy, GPU reads after submit.
        VmaAllocationCreateInfo allocInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VmaAllocationInfo allocResult;
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, &allocResult);
        be_assert(result == VK_SUCCESS, "Failed to create dynamic buffer!");

        entry.MappedPtr = allocResult.pMappedData;

        if (desc.Data) {
            memcpy(entry.MappedPtr, desc.Data, desc.Size);
        }
    } else {
        // Device-local VRAM — GPU reads fastest from here.
        // CPU uploads go through a temporary staging buffer.
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, nullptr);
        be_assert(result == VK_SUCCESS, "Failed to create device-local buffer!");

        if (desc.Data) {
            UploadToDeviceBuffer(entry.Buffer, desc.Data, desc.Size);
        }
    }

    return handle;
}

auto SenVulkanBackend::DestroyBuffer(SenBuffer handle) -> void {
    auto it = _buffers.find(handle.ID);
    if (it != _buffers.end()) {
        auto& entry = it->second;
        vmaDestroyBuffer(_allocator, entry.Buffer, entry.Allocation);
        _buffers.erase(it);
    }
}

auto SenVulkanBackend::LookupBuffer(SenBuffer handle) -> SenVulkanBufferEntry& {
    return _buffers.at(handle.ID);
}

auto SenVulkanBackend::WriteBuffer(SenBuffer handle, const void* data, uint32_t size) -> void {
    auto& entry = _buffers.at(handle.ID);
    be_assert(entry.Access != SenBufferAccess::Immutable, "Cannot write to an Immutable buffer");

    if (entry.Access == SenBufferAccess::Dynamic) {
        // Persistent-mapped: memcpy is coherent for VMA_MEMORY_USAGE_AUTO host-visible allocations.
        memcpy(entry.MappedPtr, data, size);
    } else {
        // Static: device-local, upload via staging buffer
        UploadToDeviceBuffer(entry.Buffer, data, size);
    }
}

// For Immutable/Default buffers the destination is device-local (GPU VRAM), which the CPU
// cannot write to directly. So we:
//   1. Create a temporary host-visible "staging" buffer and memcpy the data into it.
//   2. Record a one-time command buffer that does vkCmdCopyBuffer staging → dst.
//   3. Submit it, wait (fence), then destroy the staging buffer.
// This is a synchronous stall, but only happens at resource-creation time (or rarely for Default).
auto SenVulkanBackend::UploadToDeviceBuffer(VkBuffer dst, const void* data, uint32_t size) -> void {
    // 1. Create staging buffer (host-visible) via VMA
    VkBufferCreateInfo stagingInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocResult;
    VkResult result = vmaCreateBuffer(_allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocResult);
    be_assert(result == VK_SUCCESS, "Failed to create staging buffer!");

    memcpy(stagingAllocResult.pMappedData, data, size);

    // 2. One-time command buffer: copy staging → dst
    VkCommandBufferAllocateInfo cmdAlloc {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = _commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(_device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkBufferCopy copyRegion { .size = size };
    vkCmdCopyBuffer(cmd, stagingBuffer, dst, 1, &copyRegion);
    vkEndCommandBuffer(cmd);

    // 3. Submit and wait
    VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(_device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    vkQueueSubmit(_queue, 1, &submitInfo, fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(_device, fence, nullptr);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
    vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);
}
// endregion

// region ────────── samplers ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateSampler(const SenSamplerDesc& desc) -> SenSampler {
    const SenSampler handle { _nextSamplerId++ };
    auto& entry = _samplers[handle.ID];

    VkSamplerCreateInfo samplerInfo {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = Sen::Vulkan::ToFilter(desc.Filter),
        .minFilter        = Sen::Vulkan::ToFilter(desc.Filter),
        .mipmapMode       = Sen::Vulkan::ToMipmapMode(desc.Filter),
        .addressModeU     = Sen::Vulkan::ToAddressMode(desc.Address),
        .addressModeV     = Sen::Vulkan::ToAddressMode(desc.Address),
        .addressModeW     = Sen::Vulkan::ToAddressMode(desc.Address),
        .anisotropyEnable = (desc.Filter == SenFilter::Anisotropic) ? VK_TRUE : VK_FALSE,
        .maxAnisotropy    = (desc.Filter == SenFilter::Anisotropic) ? 16.f : 1.f,
        .compareEnable    = desc.Comparison ? VK_TRUE : VK_FALSE,
        .compareOp        = desc.Comparison ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_NEVER,
        .minLod           = 0.f,
        .maxLod           = VK_LOD_CLAMP_NONE,
    };

    VkResult result = vkCreateSampler(_device, &samplerInfo, nullptr, &entry.Sampler);
    be_assert(result == VK_SUCCESS, "Failed to create sampler!");

    return handle;
}

auto SenVulkanBackend::DestroySampler(SenSampler handle) -> void {
    auto it = _samplers.find(handle.ID);
    if (it != _samplers.end()) {
        vkDestroySampler(_device, it->second.Sampler, nullptr);
        _samplers.erase(it);
    }
}

auto SenVulkanBackend::LookupSampler(SenSampler handle) -> SenVulkanSamplerEntry& {
    return _samplers.at(handle.ID);
}
//endregion

// region ────────── bind groups ───────────────────────────────────────────────────────────────

auto SenVulkanBackend::CreateBindGroup(const SenBindGroupDesc& desc) -> SenBindGroup {
    // Create descriptor set layout from the bind group descriptor's slot structure
    VkDescriptorSetLayout layout = CreateDescriptorSetLayoutFromDesc(desc);

    VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = _descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };

    SenVulkanBindGroupEntry entry {};
    entry.BindGroupDesc = desc;
    VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &entry.Set);
    be_assert(result == VK_SUCCESS, "Failed to allocate descriptor set!");

    // Pre-allocate containers to avoid reallocation invalidating pointers
    size_t imageCount = desc.Textures.size() + desc.Samplers.size();
    std::vector<VkDescriptorImageInfo> imageInfos(imageCount);
    std::vector<VkDescriptorBufferInfo> bufferInfos(desc.Buffers.size());
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(imageCount + desc.Buffers.size());

    // Write textures using their slot indices
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& texture = desc.Textures[i];
        const auto& textureEntry = LookupTexture(texture);
        const uint8_t binding = desc.TextureSlots[i];

        imageInfos[i] = VkDescriptorImageInfo {
            .imageView   = textureEntry.SRV,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo      = &imageInfos[i],
        });
    }

    // Write samplers using their slot indices
    size_t samplerImageIdx = desc.Textures.size();
    for (size_t i = 0; i < desc.Samplers.size(); ++i) {
        const auto& sampler = desc.Samplers[i];
        const auto& samplerEntry = LookupSampler(sampler);
        const uint8_t binding = desc.SamplerSlots[i];

        imageInfos[samplerImageIdx] = VkDescriptorImageInfo {
            .sampler = samplerEntry.Sampler,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo      = &imageInfos[samplerImageIdx],
        });
        samplerImageIdx++;
    }

    // Write buffers using their slot indices
    for (size_t i = 0; i < desc.Buffers.size(); ++i) {
        const auto& buffer = desc.Buffers[i];
        const auto& bufferEntry = LookupBuffer(buffer);
        const uint8_t binding = desc.BufferSlots[i];

        bufferInfos[i] = VkDescriptorBufferInfo {
            .buffer = bufferEntry.Buffer,
            .offset = 0,
            .range  = bufferEntry.Size,
        };
        writes.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = entry.Set,
            .dstBinding      = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &bufferInfos[i],
        });
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(_device, uint32_t(writes.size()), writes.data(), 0, nullptr);
    }

    vkDestroyDescriptorSetLayout(_device, layout, nullptr);

    const SenBindGroup handle { _nextBindGroupId++ };
    _bindGroups[handle.ID] = entry;
    return handle;
}

auto SenVulkanBackend::DestroyBindGroup(SenBindGroup handle) -> void {
    auto it = _bindGroups.find(handle.ID);
    if (it != _bindGroups.end()) {
        vkFreeDescriptorSets(_device, _descriptorPool, 1, &it->second.Set);
        _bindGroups.erase(it);
    }
}

auto SenVulkanBackend::LookupBindGroup(SenBindGroup handle) -> SenVulkanBindGroupEntry& {
    return _bindGroups.at(handle.ID);
}

// Helper: create descriptor set layout from bind group descriptor slot structure
auto SenVulkanBackend::CreateDescriptorSetLayoutFromDesc(const SenBindGroupDesc& desc) -> VkDescriptorSetLayout {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Add texture bindings
    for (const auto& slot : desc.TextureSlots) {
        VkDescriptorSetLayoutBinding binding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        };
        bindings.push_back(binding);
    }

    // Add sampler bindings
    for (const auto& slot : desc.SamplerSlots) {
        VkDescriptorSetLayoutBinding binding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        };
        bindings.push_back(binding);
    }

    // Add buffer bindings
    for (const auto& slot : desc.BufferSlots) {
        VkDescriptorSetLayoutBinding binding {
            .binding         = slot,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags      = Sen::Vulkan::ToShaderStageFlags(desc.Stages),
        };
        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = uint32_t(bindings.size()),
        .pBindings    = bindings.data(),
    };

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorSetLayout(SenVulkanBackend::_device, &layoutInfo, nullptr, &layout);
    be_assert(result == VK_SUCCESS, "Failed to create descriptor set layout!");
    return layout;
}

// endregion

// region ────────── shaders ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreateShader(const SenShaderSourceDesc& sourceDesc) -> SenShader {
    SlangStage slangStage = SLANG_STAGE_NONE;
    switch (sourceDesc.Stage) {
        case SenShaderStage::Vertex: slangStage = SLANG_STAGE_VERTEX; break;
        case SenShaderStage::Hull:   slangStage = SLANG_STAGE_HULL;   break;
        case SenShaderStage::Domain: slangStage = SLANG_STAGE_DOMAIN; break;
        case SenShaderStage::Pixel:  slangStage = SLANG_STAGE_PIXEL;  break;
        default: be_assert(false, "SenVulkanBackend::CreateShader: unsupported shader stage"); break;
    }

    auto compileResult = SenShaderCompiler::Compile(sourceDesc.SourcePath, sourceDesc.FunctionName, slangStage, SLANG_SPIRV);
    be_assert(compileResult, "SenVulkanBackend::CreateShader: shader compilation failed: " + compileResult.error());

    auto& blob = compileResult.value();

    VkShaderModuleCreateInfo moduleInfo {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = blob->getBufferSize(),
        .pCode    = static_cast<const uint32_t*>(blob->getBufferPointer()),
    };

    const SenShader handle { _nextShaderId++ };
    auto& entry = _shaders[handle.ID];
    entry.Stage = sourceDesc.Stage;

    VkResult result = vkCreateShaderModule(_device, &moduleInfo, nullptr, &entry.Module);
    be_assert(result == VK_SUCCESS, "Failed to create shader module!");

    return handle;
}

auto SenVulkanBackend::DestroyShader(SenShader handle) -> void {
    auto it = _shaders.find(handle.ID);
    if (it != _shaders.end()) {
        vkDestroyShaderModule(_device, it->second.Module, nullptr);
        _shaders.erase(it);
    }
}

auto SenVulkanBackend::LookupShader(SenShader handle) -> SenVulkanShaderEntry& {
    return _shaders.at(handle.ID);
}

// endregion

// region ────────── pipelines ────────────────────────────────────────────────────────────────
auto SenVulkanBackend::CreatePipeline(const SenPipelineDesc& desc) -> SenPipeline {
    SenVulkanPipelineEntry entry {};
    entry.Desc = desc;

    // ── pipeline layout (from bind group layout descriptors) ──────────────────────────────
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(desc.BindGroupLayouts.size());
    for (const auto& bgl : desc.BindGroupLayouts) {
        setLayouts.push_back(CreateDescriptorSetLayoutFromDesc(bgl));
    }

    VkPipelineLayoutCreateInfo layoutInfo {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = uint32_t(setLayouts.size()),
        .pSetLayouts    = setLayouts.data(),
    };
    VkResult result = vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &entry.Layout);
    be_assert(result == VK_SUCCESS, "Failed to create pipeline layout!");

    // Clean up temporary layouts (they're kept alive by the pipeline layout)
    for (auto layout : setLayouts) {
        vkDestroyDescriptorSetLayout(_device, layout, nullptr);
    }

    // ── shader stages ──────────────────────────────────────────────────────────
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    auto addStage = [&](SenShader shader, VkShaderStageFlagBits stageBit) {
        if (!shader.IsValid()) { return; }
        stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = stageBit,
            .module = LookupShader(shader).Module,
            .pName  = "main",
        });
    };
    addStage(desc.VertexShader, VK_SHADER_STAGE_VERTEX_BIT);
    addStage(desc.HullShader,   VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    addStage(desc.DomainShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    addStage(desc.PixelShader,  VK_SHADER_STAGE_FRAGMENT_BIT);

    // ── vertex input ───────────────────────────────────────────────────────────
    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.VertexLayout.size());
    for (const auto& elem : desc.VertexLayout) {
        attributes.push_back(VkVertexInputAttributeDescription {
            .location = elem.Location,
            .binding  = 0,
            .format   = Sen::Vulkan::ToFormat(elem.Format),
            .offset   = elem.Offset,
        });
    }

    uint32_t vertexStride = desc.VertexStride;

    VkVertexInputBindingDescription binding {
        .binding   = 0,
        .stride    = vertexStride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkPipelineVertexInputStateCreateInfo vertexInput {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = desc.VertexLayout.empty() ? 0u : 1u,
        .pVertexBindingDescriptions      = desc.VertexLayout.empty() ? nullptr : &binding,
        .vertexAttributeDescriptionCount = uint32_t(attributes.size()),
        .pVertexAttributeDescriptions    = attributes.data(),
    };

    // ── input assembly ─────────────────────────────────────────────────────────
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = Sen::Vulkan::ToTopology(desc.Topology),
        .primitiveRestartEnable = VK_FALSE,
    };

    // ── tessellation ───────────────────────────────────────────────────────────
    VkPipelineTessellationStateCreateInfo tessellation {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = desc.Topology == SenTopology::PatchList3 ? 3u : 1u,
    };

    // ── viewport (dynamic) ─────────────────────────────────────────────────────
    VkPipelineViewportStateCreateInfo viewportState {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    // ── rasterizer ─────────────────────────────────────────────────────────────
    VkPipelineRasterizationStateCreateInfo rasterizer {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = !desc.RasterizerState.DepthClipEnable,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = Sen::Vulkan::ToFillMode(desc.RasterizerState.FillMode),
        .cullMode                = Sen::Vulkan::ToCullMode(desc.RasterizerState.CullMode),
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = desc.RasterizerState.DepthBias != 0.f || desc.RasterizerState.SlopeScaledDepthBias != 0.f,
        .depthBiasConstantFactor = desc.RasterizerState.DepthBias,
        .depthBiasSlopeFactor    = desc.RasterizerState.SlopeScaledDepthBias,
        .lineWidth               = 1.f,
    };

    // ── multisample ────────────────────────────────────────────────────────────
    VkPipelineMultisampleStateCreateInfo multisample {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // ── depth stencil ──────────────────────────────────────────────────────────
    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = desc.DepthStencilState.DepthEnable,
        .depthWriteEnable = desc.DepthStencilState.DepthWriteEnable,
        .depthCompareOp   = Sen::Vulkan::ToCompareOp(desc.DepthStencilState.DepthFunc),
    };

    // ── blend ──────────────────────────────────────────────────────────────────
    uint32_t rtCount = uint32_t(desc.RenderTargetFormats.size());
    VkPipelineColorBlendAttachmentState blendAttachment {
        .blendEnable         = desc.BlendState.Enable,
        .srcColorBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.SrcBlend),
        .dstColorBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.DstBlend),
        .colorBlendOp        = Sen::Vulkan::ToBlendOp(desc.BlendState.BlendOp),
        .srcAlphaBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.SrcBlendAlpha),
        .dstAlphaBlendFactor = Sen::Vulkan::ToBlendFactor(desc.BlendState.DstBlendAlpha),
        .alphaBlendOp        = Sen::Vulkan::ToBlendOp(desc.BlendState.BlendOpAlpha),
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(rtCount, blendAttachment);

    VkPipelineColorBlendStateCreateInfo blendState {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = uint32_t(blendAttachments.size()),
        .pAttachments    = blendAttachments.data(),
    };

    // ── dynamic state ──────────────────────────────────────────────────────────
    std::array<VkDynamicState, 2> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = uint32_t(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data(),
    };

    // ── dynamic rendering ──────────────────────────────────────────────────────
    std::vector<VkFormat> colorFormats;
    colorFormats.reserve(desc.RenderTargetFormats.size());
    for (const auto& fmt : desc.RenderTargetFormats) {
        colorFormats.push_back(Sen::Vulkan::ToFormat(fmt));
    }
    VkFormat depthFormat = Sen::Vulkan::ToFormat(desc.DepthStencilFormat);

    VkPipelineRenderingCreateInfoKHR renderingInfo {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = uint32_t(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat   = depthFormat,
    };

    // ── create pipeline ────────────────────────────────────────────────────────
    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingInfo,
        .stageCount          = uint32_t(stages.size()),
        .pStages             = stages.data(),
        .pVertexInputState   = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState  = &tessellation,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisample,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &blendState,
        .pDynamicState       = &dynamicState,
        .layout              = entry.Layout,
    };

    result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &entry.Pipeline);
    be_assert(result == VK_SUCCESS, "Failed to create graphics pipeline!");

    const SenPipeline handle { _nextPipelineId++ };
    _pipelines[handle.ID] = entry;
    return handle;
}

auto SenVulkanBackend::DestroyPipeline(SenPipeline handle) -> void {
    auto it = _pipelines.find(handle.ID);
    if (it != _pipelines.end()) {
        vkDestroyPipeline(_device, it->second.Pipeline, nullptr);
        vkDestroyPipelineLayout(_device, it->second.Layout, nullptr);
        _pipelines.erase(it);
    }
}

auto SenVulkanBackend::LookupPipeline(SenPipeline handle) -> SenVulkanPipelineEntry& {
    return _pipelines.at(handle.ID);
}

// endregion

// region ────────── debug print helpers ───────────────────────────────────────────────────────────

auto SenVulkanBackend::PrintBindGroup(SenBindGroup handle) -> std::string {
    std::string s;
    s += std::format("[BindGroup] handle.ID={} valid={}\n", handle.ID, handle.IsValid());
    if (!handle.IsValid()) { return s; }

    auto& entry = _bindGroups.at(handle.ID);
    s += std::format("\tVkDescriptorSet = {}\n", (void*)entry.Set);

    const auto& desc = entry.BindGroupDesc;

    s += std::format("\tLayout structure:\n");
    s += std::format("\t\tTextureSlots ({}):\n", desc.TextureSlots.size());
    for (const auto& slot : desc.TextureSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }
    s += std::format("\t\tSamplerSlots ({}):\n", desc.SamplerSlots.size());
    for (const auto& slot : desc.SamplerSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }
    s += std::format("\t\tBufferSlots ({}):\n", desc.BufferSlots.size());
    for (const auto& slot : desc.BufferSlots) {
        s += std::format("\t\t\tslot={}\n", slot);
    }

    s += std::format("\tTextures ({}):\n", desc.Textures.size());
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& tex = desc.Textures[i];
        s += std::format("\t\t[{}] SenTexture.ID={} valid={}\n", i, tex.ID, tex.IsValid());
        if (tex.IsValid()) {
            const auto& t = _textures.at(tex.ID);
            s += std::format("\t\t\tVkImage       = {}\n", (void*)t.Image);
            s += std::format("\t\t\tSRV           = {}\n", (void*)t.SRV);
            s += std::format("\t\t\tVkFormat      = {}\n", (uint32_t)t.Format);
            s += std::format("\t\t\tCurrentLayout = {}\n", (uint32_t)t.CurrentLayout);
        }
    }

    s += std::format("\tSamplers ({}):\n", desc.Samplers.size());
    for (size_t i = 0; i < desc.Samplers.size(); ++i) {
        const auto& smp = desc.Samplers[i];
        s += std::format("\t\t[{}] SenSampler.ID={} valid={}\n", i, smp.ID, smp.IsValid());
        if (smp.IsValid()) {
            const auto& s2 = _samplers.at(smp.ID);
            s += std::format("\t\t\tVkSampler = {}\n", (void*)s2.Sampler);
        }
    }

    s += std::format("\tBuffers ({}):\n", desc.Buffers.size());
    for (size_t i = 0; i < desc.Buffers.size(); ++i) {
        const auto& buf = desc.Buffers[i];
        s += std::format("\t\t[{}] SenBuffer.ID={} valid={}\n", i, buf.ID, buf.IsValid());
        if (buf.IsValid()) {
            const auto& b = _buffers.at(buf.ID);
            s += std::format("\t\t\tVkBuffer = {}\n", (void*)b.Buffer);
            s += std::format("\t\t\tSize     = {}\n", b.Size);
            s += std::format("\t\t\tAccess   = {}\n", (uint32_t)b.Access);
        }
    }

    s += std::format("\tImageTextures for auto-barrier ({}):\n", desc.Textures.size());
    for (size_t i = 0; i < desc.Textures.size(); ++i) {
        const auto& tex = desc.Textures[i];
        s += std::format("\t\t[{}] SenTexture.ID={} valid={}\n", i, tex.ID, tex.IsValid());
        if (tex.IsValid()) {
            const auto& t = _textures.at(tex.ID);
            s += std::format("\t\t\tCurrentLayout = {}\n", (uint32_t)t.CurrentLayout);
        }
    }

    return s;
}

// endregion
