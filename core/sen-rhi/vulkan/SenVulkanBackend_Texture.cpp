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
        for (auto v : entry.MipSRVs) { destroy(v); }
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
    entry.Format     = format;
    entry.Width      = desc.Width;
    entry.Height     = desc.Height;
    entry.MipLevels  = desc.Mips;
    entry.LayerCount = 1;
    entry.MipLayouts.assign(desc.Mips, VK_IMAGE_LAYOUT_UNDEFINED);

    // Mip generation blits each level into the next, so every level is both a transfer source and destination.
    const VkImageUsageFlags mipUsage = desc.Mips > 1 ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) : 0;

    VkImageCreateInfo imageInfo {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { desc.Width, desc.Height, 1 },
        .mipLevels     = desc.Mips,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage | mipUsage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
    VkResult result = vmaCreateImage(_allocator, &imageInfo, &allocInfo, &entry.Image, &entry.Allocation, nullptr);
    be_assert(result == VK_SUCCESS, "Failed to create 2D image!");

    if (desc.Data) {
        const uint32_t dataSize = desc.Width * desc.Height * Sen::Vulkan::BytesPerPixel(desc.Format);
        UploadToDeviceImage(entry.Image, aspect, desc.Data, dataSize, desc.Width, desc.Height, desc.Mips, 1);
        entry.MipLayouts.assign(desc.Mips, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (HasAny(desc.Usage, SenTextureUsage::ShaderResource)) {
        entry.SRV = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, desc.Mips, 0, 1);
        entry.MipSRVs.resize(desc.Mips);
        for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
            entry.MipSRVs[mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, 0, 1);
        }
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
    entry.Format     = format;
    entry.Width      = desc.Width;
    entry.Height     = desc.Height;
    entry.MipLevels  = desc.Mips;
    entry.LayerCount = 6;
    entry.MipLayouts.assign(desc.Mips, VK_IMAGE_LAYOUT_UNDEFINED);

    // Mip generation blits each level into the next, so every level is both a transfer source and destination.
    const VkImageUsageFlags mipUsage = desc.Mips > 1 ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) : 0;

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
        .usage         = usage | mipUsage,
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
        entry.MipLayouts.assign(desc.Mips, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

auto SenVulkanBackend::GenerateMips(SenTexture handle) -> void {
    auto& entry = _textures.at(handle.ID);
    be_assert(entry.MipLevels > 1, "GenerateMips: texture has only one mip level");

    // vkCmdBlitImage downsamples with a linear filter — the format must advertise linear-filter support.
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(_physicalDevice, entry.Format, &formatProps);
    be_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
              "GenerateMips: texture format does not support linear blit filtering");

    const VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    const uint32_t           layers = entry.LayerCount;

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

    auto barrier = [&](uint32_t baseMip, uint32_t levelCount, VkImageLayout oldLayout, VkImageLayout newLayout,
                       VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                       VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) -> void {
        const VkImageSubresourceRange range { aspect, baseMip, levelCount, 0, layers };
        RecordImageBarrier(cmd, MakeImageBarrier(
            entry.Image, range, oldLayout, newLayout, srcStage, srcAccess, dstStage, dstAccess));
    };

    // Bring every level into TRANSFER_DST. Mip 0 already holds the uploaded image; the rest are scratch.
    // GenerateMips runs right after upload, so all mips share one layout — read level 0 as the source layout.
    barrier(0, entry.MipLevels, entry.MipLayouts[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    int32_t mipWidth  = static_cast<int32_t>(entry.Width);
    int32_t mipHeight = static_cast<int32_t>(entry.Height);
    for (uint32_t mip = 1; mip < entry.MipLevels; ++mip) {
        // Flip the source level (mip-1) to TRANSFER_SRC and wait for its prior write to complete.
        barrier(mip - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);

        const int32_t nextWidth  = mipWidth  > 1 ? mipWidth  / 2 : 1;
        const int32_t nextHeight = mipHeight > 1 ? mipHeight / 2 : 1;

        VkImageBlit blit {
            .srcSubresource = { aspect, mip - 1, 0, layers },
            .srcOffsets     = { { 0, 0, 0 }, { mipWidth, mipHeight, 1 } },
            .dstSubresource = { aspect, mip, 0, layers },
            .dstOffsets     = { { 0, 0, 0 }, { nextWidth, nextHeight, 1 } },
        };
        vkCmdBlitImage(cmd, entry.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            entry.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1, &blit, VK_FILTER_LINEAR);

        mipWidth  = nextWidth;
        mipHeight = nextHeight;
    }

    // Levels [0, last) ended as TRANSFER_SRC; the last level is still TRANSFER_DST. Move all to shader-read.
    barrier(0, entry.MipLevels - 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    barrier(entry.MipLevels - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

    entry.MipLayouts.assign(entry.MipLevels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(_device, &fenceInfo, nullptr, &fence);
    VkSubmitInfo submitInfo { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd };
    vkQueueSubmit(_queue, 1, &submitInfo, fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(_device, fence, nullptr);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
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

    RecordImageBarrier(cmd, MakeImageBarrier(image, { aspect, 0, mipLevels, 0, layerCount },
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

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

    RecordImageBarrier(cmd, MakeImageBarrier(image, { aspect, 0, mipLevels, 0, layerCount },
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    ));

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

auto SenVulkanBackend::MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout,
                                        VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                                        VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) -> VkImageMemoryBarrier2 {
    return VkImageMemoryBarrier2 {
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
        .subresourceRange    = range,
    };
}

auto SenVulkanBackend::MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout) -> VkImageMemoryBarrier2 {
    VkPipelineStageFlags2 srcStage, dstStage;
    VkAccessFlags2        srcAccess, dstAccess;
    Sen::Vulkan::ScopeForLayout(oldLayout, srcStage, srcAccess);
    Sen::Vulkan::ScopeForLayout(newLayout, dstStage, dstAccess);
    return MakeImageBarrier(image, range, oldLayout, newLayout, srcStage, srcAccess, dstStage, dstAccess);
}

auto SenVulkanBackend::RecordImageBarrier(VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier) -> void {
    const VkDependencyInfo dependency {
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
