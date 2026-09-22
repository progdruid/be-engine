#include "SenVulkanState.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

using namespace SenVk;

auto Sen::CreateTexture(const SenTextureDesc& desc) -> SenTexture {
    auto entry = SenVulkanTextureEntry();

    const VkFormat format = SenVk::ToFormat(desc.Format);
    const VkImageUsageFlags usage = SenVk::ToImageUsageFlags(desc.Usage);

    const uint32_t cubeFactor = desc.Cubemap ? 6 : 1;
    const uint32_t layerCount = cubeFactor * (desc.ArrayLength > 0 ? desc.ArrayLength : 1);

    entry.Format     = format;
    entry.Width      = desc.Width;
    entry.Height     = desc.Height;
    entry.MipLevels  = desc.Mips;
    entry.LayerCount = layerCount;

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

    return _textures.Create(std::move(entry));
}

auto Sen::DestroyTexture(SenTexture handle) -> void {
    if (!_textures.Contains(handle)) {
        return;
    }

    auto& entry = _textures.Get(handle);
    vmaDestroyImage(_allocator, entry.Image, entry.Allocation);
    _textures.Destroy(handle);
}

auto SenVk::LookupTexture(SenTexture handle) -> SenVulkanTextureEntry& {
    return _textures.Get(handle);
}

auto SenVk::MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout,
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

auto SenVk::MakeImageBarrier(VkImage image, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout) -> VkImageMemoryBarrier2 {
    VkPipelineStageFlags2 srcStage, dstStage;
    VkAccessFlags2        srcAccess, dstAccess;
    SenVk::ScopeForLayout(oldLayout, srcStage, srcAccess);
    SenVk::ScopeForLayout(newLayout, dstStage, dstAccess);
    return MakeImageBarrier(image, range, oldLayout, newLayout, srcStage, srcAccess, dstStage, dstAccess);
}

auto SenVk::RecordImageBarrier(VkCommandBuffer cmd, const VkImageMemoryBarrier2& barrier) -> void {
    const VkDependencyInfo dependency {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier,
    };
    vkCmdPipelineBarrier2(cmd, &dependency);
}

auto SenVk::CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspect, uint32_t baseMip, uint32_t mipLevels, uint32_t baseLayer, uint32_t layerCount) -> VkImageView {
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
