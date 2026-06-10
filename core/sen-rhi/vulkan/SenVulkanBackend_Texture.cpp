#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

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

auto SenVulkanBackend:: LookupTexture(SenTexture handle) -> SenVulkanTextureEntry& {
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
        std::vector<uint8_t> expanded(faceSize * 6);
        for (int face = 0; face < 6; ++face)
            memcpy(expanded.data() + face * faceSize, desc.Data, faceSize);
        UploadToDeviceImage(entry.Image, aspect, expanded.data(), faceSize * 6, desc.Width, desc.Height, desc.Mips, 6);
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

    TransitionRawImageLayout(cmd, image, aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, layerCount);

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

    TransitionRawImageLayout(cmd, image, aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, layerCount);

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

auto SenVulkanBackend::TransitionRawImageLayout(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount) -> void {
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
                be_assert(false, "TransitionRawImageLayout: unsupported layout");
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
