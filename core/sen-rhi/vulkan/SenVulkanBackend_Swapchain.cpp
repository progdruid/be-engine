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
        texEntry.MipLayouts.assign(1, VK_IMAGE_LAYOUT_UNDEFINED);

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

    entry.FramesInFlight = desc.FramesInFlight;
    entry.ImageAvailableSemaphores.resize(entry.FramesInFlight);
    entry.InFlightFences.resize(entry.FramesInFlight);
    for (uint32_t i = 0; i < entry.FramesInFlight; ++i) {
        result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.ImageAvailableSemaphores[i]);
        be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
        result = vkCreateFence(_device, &fenceInfo, nullptr, &entry.InFlightFences[i]);
        be_assert(result == VK_SUCCESS, "Failed to create fence!");
    }

    entry.RenderFinishedSemaphores.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &entry.RenderFinishedSemaphores[i]);
        be_assert(result == VK_SUCCESS, "Failed to create semaphore!");
    }
    entry.ImagesInFlight.assign(imageCount, VK_NULL_HANDLE);

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

        for (auto fence : entry.InFlightFences)               { vkDestroyFence(_device, fence, nullptr); }
        for (auto semaphore : entry.ImageAvailableSemaphores) { vkDestroySemaphore(_device, semaphore, nullptr); }
        for (auto semaphore : entry.RenderFinishedSemaphores) { vkDestroySemaphore(_device, semaphore, nullptr); }
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
        .FramesInFlight = entry.FramesInFlight,
        .Format = entry.Format,
        .PresentMode = entry.PresentMode,
    });
}

auto SenVulkanBackend::GetSwapchainFormat(SenSwapchain handle) -> SenFormat {
    return _swapchains.at(handle.ID).Format;
}

auto SenVulkanBackend::GetSwapchainWidth(SenSwapchain handle) -> uint32_t {
    return _swapchains.at(handle.ID).Width;
}

auto SenVulkanBackend::GetSwapchainHeight(SenSwapchain handle) -> uint32_t {
    return _swapchains.at(handle.ID).Height;
}

auto SenVulkanBackend::BeginFrame(SenSwapchain handle, uint32_t frameSlot) -> SenTexture {
    auto& entry = _swapchains.at(handle.ID);
    be_assert(frameSlot < entry.FramesInFlight, "BeginFrame: frame slot out of range");

    vkWaitForFences(_device, 1, &entry.InFlightFences[frameSlot], VK_TRUE, UINT64_MAX);

    vkAcquireNextImageKHR(
        _device, entry.Swapchain, UINT64_MAX,
        entry.ImageAvailableSemaphores[frameSlot], VK_NULL_HANDLE,
        &entry.CurrentImageIndex
    );

    if (entry.ImagesInFlight[entry.CurrentImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &entry.ImagesInFlight[entry.CurrentImageIndex], VK_TRUE, UINT64_MAX);
    }
    entry.ImagesInFlight[entry.CurrentImageIndex] = entry.InFlightFences[frameSlot];

    vkResetFences(_device, 1, &entry.InFlightFences[frameSlot]);

    return entry.Textures[entry.CurrentImageIndex];
}

auto SenVulkanBackend::EndFrame(SenSwapchain handle, SenVulkanCommandBuffer& cmd, uint32_t frameSlot) -> void {
    auto& entry = _swapchains.at(handle.ID);
    be_assert(frameSlot < entry.FramesInFlight, "EndFrame: frame slot out of range");

    const VkCommandBuffer vkCmd = cmd.GetNativeHandle();

    // Submit: wait on imageAvailable, signal renderFinished, signal fence when done
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &entry.ImageAvailableSemaphores[frameSlot],
        .pWaitDstStageMask    = &waitStage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &vkCmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &entry.RenderFinishedSemaphores[entry.CurrentImageIndex],
    };
    vkQueueSubmit(_queue, 1, &submitInfo, entry.InFlightFences[frameSlot]);

    // Present: wait on renderFinished
    VkPresentInfoKHR presentInfo {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &entry.RenderFinishedSemaphores[entry.CurrentImageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &entry.Swapchain,
        .pImageIndices      = &entry.CurrentImageIndex,
    };
    vkQueuePresentKHR(_queue, &presentInfo);
}
