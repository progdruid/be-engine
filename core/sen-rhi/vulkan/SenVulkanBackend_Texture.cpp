#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateTexture(const SenTextureDesc& desc) -> SenTexture {
    auto entry = SenVulkanTextureEntry();

    const VkFormat           format  = Sen::Vulkan::ToFormat(desc.Format);
    const VkImageUsageFlags  usage   = Sen::Vulkan::ToImageUsageFlags(desc.Usage);
    const bool               isDepth = HasAny(desc.Usage, SenTextureUsage::DepthStencil);
    const VkImageAspectFlags aspect  = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    const uint32_t cubeFactor = desc.Cubemap ? 6 : 1;
    const uint32_t layerCount = cubeFactor * (desc.ArrayLength > 0 ? desc.ArrayLength : 1);
    const bool     isLayered  = layerCount > 1;

    entry.Format     = format;
    entry.Width      = desc.Width;
    entry.Height     = desc.Height;
    entry.MipLevels  = desc.Mips;
    entry.LayerCount = layerCount;
    entry.MipLayouts.assign(desc.Mips, VK_IMAGE_LAYOUT_UNDEFINED);

    VkImageCreateInfo imageInfo {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = desc.Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : VkImageCreateFlags(0),
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = { desc.Width, desc.Height, 1 },
        .mipLevels     = desc.Mips,
        .arrayLayers   = layerCount,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
    VkResult result = vmaCreateImage(_allocator, &imageInfo, &allocInfo, &entry.Image, &entry.Allocation, nullptr);
    be_assert(result == VK_SUCCESS, "Failed to create image!");

    if (HasAny(desc.Usage, SenTextureUsage::ShaderResource)) {
        const VkImageViewType srvType =
            desc.Cubemap ? (desc.ArrayLength > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE)
                         : (isLayered            ? VK_IMAGE_VIEW_TYPE_2D_ARRAY    : VK_IMAGE_VIEW_TYPE_2D);
        entry.SRV = CreateImageView(entry.Image, format, srvType, aspect, 0, desc.Mips, 0, layerCount);
        HeapRegisterTexture(entry, entry.SRV, srvType);
        if (!isLayered) {
            entry.MipSRVs.resize(desc.Mips);
            entry.MipHeapIndices.resize(desc.Mips, UINT32_MAX);
            for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
                entry.MipSRVs[mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, 0, 1);
                entry.MipHeapIndices[mip] = HeapRegisterView(SenHeapBinding::Texture2D, entry.MipSRVs[mip]);
            }
        }
    }
    if (HasAny(desc.Usage, SenTextureUsage::DepthStencil)) {
        if (isLayered) {
            entry.LayerDSVs.resize(layerCount);
            for (uint32_t layer = 0; layer < layerCount; ++layer) {
                entry.LayerDSVs[layer] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, 1, layer, 1);
            }
        } else {
            entry.DSV = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, 0, 1, 0, 1);
        }
    }
    if (HasAny(desc.Usage, SenTextureUsage::RenderTarget)) {
        if (isLayered) {
            entry.LayerMipRTVs.resize(layerCount);
            for (uint32_t layer = 0; layer < layerCount; ++layer) {
                entry.LayerMipRTVs[layer].resize(desc.Mips);
                for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
                    entry.LayerMipRTVs[layer][mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, layer, 1);
                }
            }
        } else {
            entry.MipRTVs.resize(desc.Mips);
            for (uint32_t mip = 0; mip < desc.Mips; ++mip) {
                entry.MipRTVs[mip] = CreateImageView(entry.Image, format, VK_IMAGE_VIEW_TYPE_2D, aspect, mip, 1, 0, 1);
            }
        }
    }

    return _textures.Create(std::move(entry));
}

auto SenVulkanBackend::DestroyTexture(SenTexture handle) -> void {
    if (!_textures.Contains(handle)) {
        return;
    }

    auto& entry = _textures.Get(handle);
    HeapReleaseTexture(entry);

    auto destroy = [&](VkImageView view) -> void { if (view) { vkDestroyImageView(_device, view, nullptr); } };
    destroy(entry.SRV);
    destroy(entry.DSV);
    for (auto view : entry.MipSRVs) { destroy(view); }
    for (auto view : entry.MipRTVs) { destroy(view); }
    for (auto view : entry.LayerDSVs) { destroy(view); }
    for (auto& mips : entry.LayerMipRTVs) { for (auto view : mips) { destroy(view); } }

    vmaDestroyImage(_allocator, entry.Image, entry.Allocation);
    _textures.Destroy(handle);
}

auto SenVulkanBackend:: LookupTexture(SenTexture handle) -> SenVulkanTextureEntry& {
    return _textures.Get(handle);
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
