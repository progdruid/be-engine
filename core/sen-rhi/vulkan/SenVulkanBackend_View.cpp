#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

namespace {
    auto ToImageViewType(SenViewType type) -> VkImageViewType {
        switch (type) {
            case SenViewType::Texture2D:        return VK_IMAGE_VIEW_TYPE_2D;
            case SenViewType::Texture2DArray:   return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case SenViewType::TextureCube:      return VK_IMAGE_VIEW_TYPE_CUBE;
            case SenViewType::TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }
        be_assert(false, "SenVulkanBackend: unknown view type");
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

auto SenVulkanBackend::CreateView(const SenViewDesc& desc) -> SenView {
    auto& texture = _textures.Get(desc.Texture);

    const bool isDepth = texture.Format == VK_FORMAT_D32_SFLOAT;
    const VkImageAspectFlags aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    auto entry = SenVulkanViewEntry();
    entry.Desc = desc;
    entry.View = CreateImageView(
        texture.Image, texture.Format, ToImageViewType(desc.Type), aspect,
        desc.BaseMip, desc.MipCount, desc.BaseLayer, desc.LayerCount
    );

    return _views.Create(std::move(entry));
}

auto SenVulkanBackend::DestroyView(SenView handle) -> void {
    if (!_views.Contains(handle)) {
        return;
    }

    auto& entry = _views.Get(handle);
    vkDestroyImageView(_device, entry.View, nullptr);
    _views.Destroy(handle);
}

auto SenVulkanBackend::LookupView(SenView handle) -> SenVulkanViewEntry& {
    return _views.Get(handle);
}

auto SenVulkanBackend::GetViewDesc(SenView handle) -> const SenViewDesc& {
    return _views.Get(handle).Desc;
}

auto SenVulkanBackend::GetViewFormat(SenView handle) -> SenFormat {
    const auto& desc = _views.Get(handle).Desc;
    return SenVk::FromVkFormat(_textures.Get(desc.Texture).Format);
}
