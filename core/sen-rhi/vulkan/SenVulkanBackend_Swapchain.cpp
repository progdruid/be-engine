#include "SenVulkanBackend.h"

// TODO: surface creation currently delegates to GLFW, which ties sen-rhi to a windowing library.
// The correct fix is a small platform-dispatch service inside sen that, given (platform, display server, API choice),
// returns the appropriate surface + required instance extensions — with no windowing lib
// knowledge at the sen level. Until that service exists, GLFW is used here as a stopgap.
#include <umbrellas/include-glfw.h>
#include <sen-rhi/vulkan/SenVulkanConvert.h>

#include <umbrellas/include-libassert.h>

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

    {
        static const auto ModeName = [](VkPresentModeKHR mode) -> const char* {
            switch (mode) {
                case VK_PRESENT_MODE_IMMEDIATE_KHR:    return "IMMEDIATE";
                case VK_PRESENT_MODE_MAILBOX_KHR:      return "MAILBOX";
                case VK_PRESENT_MODE_FIFO_KHR:         return "FIFO";
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
                default:                               return "?";
            }
        };
        const char* requested = "FIFO";
        if (desc.PresentMode == SenPresentMode::Immediate) { requested = "IMMEDIATE"; }
        if (desc.PresentMode == SenPresentMode::Mailbox)   { requested = "MAILBOX"; }

        std::fprintf(stderr, "[vulkan] present mode requested=%s chosen=%s   supported:", requested, ModeName(chosenPresentMode));
        for (const auto& mode : presentModes) {
            std::fprintf(stderr, " %s", ModeName(mode));
        }
        std::fprintf(stderr, "\n");
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

    result = vkCreateSwapchainKHR(_device, &swapchainInfo, nullptr, &entry.Swapchain);
    be_assert(result == VK_SUCCESS, "Failed to create swapchain!");

    // 4. Get swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(_device, entry.Swapchain, &imageCount, nullptr);
    entry.Images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, entry.Swapchain, &imageCount, entry.Images.data());

    std::fprintf(stderr, "[vulkan] swapchain images requested=%u actual=%u  extent=%ux%u\n",
        minImageCount, imageCount, imageExtent.width, imageExtent.height);

    entry.NativeWindowHandle = desc.NativeWindowHandle;
    entry.Width       = imageExtent.width;
    entry.Height      = imageExtent.height;
    entry.BufferCount = desc.BufferCount;
    entry.Format      = SenVk::FromVkFormat(chosenFormat.format);
    entry.PresentMode = desc.PresentMode;

    // 5. Register each swapchain image as a SenTexture plus its attachment view
    entry.Textures.resize(imageCount);
    entry.Views.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        SenVulkanTextureEntry texEntry {};
        texEntry.Image  = entry.Images[i];
        texEntry.Format = chosenFormat.format;
        texEntry.Width  = imageExtent.width;
        texEntry.Height = imageExtent.height;

        entry.Textures[i] = _textures.Create(std::move(texEntry));
        entry.Views[i] = CreateView({
            .Texture = entry.Textures[i],
            .Type = SenViewType::Texture2D,
        });
    }

    // 6. Create sync objects
    VkSemaphoreCreateInfo semaphoreInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    const uint32_t acquireCount = imageCount + 1;
    entry.AcquireSemaphores.resize(acquireCount);
    entry.AcquireTimelineValues.assign(acquireCount, 0);
    for (uint32_t i = 0; i < acquireCount; ++i) {
        result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.AcquireSemaphores[i]);
        be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
    }

    entry.RenderFinishedSemaphores.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.RenderFinishedSemaphores[i]);
        be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
    }
    entry.ImageTimelineValues.assign(imageCount, 0);

    return _swapchains.Create(std::move(entry));
}

auto SenVulkanBackend::DestroySwapchain(SenSwapchain handle) -> void {
    if (!_swapchains.Contains(handle)) {
        return;
    }

    auto& entry = _swapchains.Get(handle);

    // The images belong to the swapchain, so only their views and slot entries are ours to free.
    for (const auto& view : entry.Views)                  { DestroyView(view); }
    for (const auto& tex : entry.Textures)                { _textures.Destroy(tex); }
    for (auto semaphore : entry.AcquireSemaphores)        { vkDestroySemaphore(_device, semaphore, nullptr); }
    for (auto semaphore : entry.RenderFinishedSemaphores) { vkDestroySemaphore(_device, semaphore, nullptr); }
    if (entry.Swapchain) { vkDestroySwapchainKHR(_device, entry.Swapchain, nullptr); }
    if (entry.Surface)   { vkDestroySurfaceKHR(_instance, entry.Surface, nullptr); }

    _swapchains.Destroy(handle);
}

auto SenVulkanBackend::ResizeSwapchain(SenSwapchain& handle, uint32_t width, uint32_t height) -> void {
    const auto& entry = _swapchains.Get(handle);
    const SenSwapchainDesc desc {
        .NativeWindowHandle = entry.NativeWindowHandle,
        .Width = width,
        .Height = height,
        .BufferCount = entry.BufferCount,
        .Format = entry.Format,
        .PresentMode = entry.PresentMode,
    };

    DestroySwapchain(handle);
    handle = CreateSwapchain(desc);
}

auto SenVulkanBackend::GetSwapchainFormat(SenSwapchain handle) -> SenFormat {
    return _swapchains.Get(handle).Format;
}

auto SenVulkanBackend::GetSwapchainWidth(SenSwapchain handle) -> uint32_t {
    return _swapchains.Get(handle).Width;
}

auto SenVulkanBackend::GetSwapchainHeight(SenSwapchain handle) -> uint32_t {
    return _swapchains.Get(handle).Height;
}

auto SenVulkanBackend::GetSurfaceExtent(SenSwapchain handle, uint32_t& outWidth, uint32_t& outHeight) -> void {
    const auto& entry = _swapchains.Get(handle);
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, entry.Surface, &capabilities);
    outWidth  = capabilities.currentExtent.width;
    outHeight = capabilities.currentExtent.height;
}

auto SenVulkanBackend::AcquireSwapchainView(SenSwapchain handle) -> SenView {
    auto& entry = _swapchains.Get(handle);

    const uint32_t acquireIndex = entry.AcquireIndex;
    const uint64_t acquireValue = entry.AcquireTimelineValues[acquireIndex];
    const VkSemaphoreWaitInfo acquireWait {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &_timeline,
        .pValues        = &acquireValue,
    };
    vkWaitSemaphores(_device, &acquireWait, UINT64_MAX);

    const VkResult acquireResult = vkAcquireNextImageKHR(
        _device, entry.Swapchain, UINT64_MAX,
        entry.AcquireSemaphores[acquireIndex], VK_NULL_HANDLE,
        &entry.CurrentImageIndex
    );
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return SenView{};
    }

    entry.PendingAcquireIndex = acquireIndex;
    entry.AcquireIndex = (acquireIndex + 1) % uint32_t(entry.AcquireSemaphores.size());

    const uint64_t imageValue = entry.ImageTimelineValues[entry.CurrentImageIndex];
    const VkSemaphoreWaitInfo imageWait {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &_timeline,
        .pValues        = &imageValue,
    };
    vkWaitSemaphores(_device, &imageWait, UINT64_MAX);

    return entry.Views[entry.CurrentImageIndex];
}

