#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

namespace {
    auto ToImageViewType(SenViewType type) -> VkImageViewType {
        switch (type) {
            case SenViewType::Sampled2D:        return VK_IMAGE_VIEW_TYPE_2D;
            case SenViewType::Sampled2DArray:   return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case SenViewType::SampledCube:      return VK_IMAGE_VIEW_TYPE_CUBE;
            case SenViewType::SampledCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            case SenViewType::Storage2D:        return VK_IMAGE_VIEW_TYPE_2D;
            case SenViewType::Attachment2D:     return VK_IMAGE_VIEW_TYPE_2D;
        }
        be_assert(false, "SenVulkanBackend: unknown view type");
        return VK_IMAGE_VIEW_TYPE_2D;
    }

    auto ToHeapBinding(SenViewType type) -> SenHeapBinding {
        switch (type) {
            case SenViewType::Sampled2D:        return SenHeapBinding::Texture2D;
            case SenViewType::Sampled2DArray:   return SenHeapBinding::Texture2DArray;
            case SenViewType::SampledCube:      return SenHeapBinding::TextureCube;
            case SenViewType::SampledCubeArray: return SenHeapBinding::TextureCubeArray;
            case SenViewType::Storage2D:        return SenHeapBinding::StorageTexture2D;
            default: break;
        }
        be_assert(false, "SenVulkanBackend: view type has no heap binding");
        return SenHeapBinding::Texture2D;
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

    // Attachment views are addressed by the render pass, never by a shader, so they take no heap slot.
    if (desc.Type != SenViewType::Attachment2D) {
        const SenHeapBinding binding = ToHeapBinding(desc.Type);
        entry.HeapBinding = uint32_t(binding);
        entry.HeapIndex = HeapRegisterView(binding, entry.View);
    }

    return _views.Create(std::move(entry));
}

auto SenVulkanBackend::DestroyView(SenView handle) -> void {
    if (!_views.Contains(handle)) {
        return;
    }

    auto& entry = _views.Get(handle);
    if (entry.HeapIndex != UINT32_MAX) {
        _heapFree[entry.HeapBinding].push_back(entry.HeapIndex);
    }
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
    return Sen::Vulkan::FromVkFormat(_textures.Get(desc.Texture).Format);
}

auto SenVulkanBackend::GetViewHeapIndex(SenView handle) -> uint32_t {
    const auto& entry = _views.Get(handle);
    be_assert(entry.HeapIndex != UINT32_MAX, "GetViewHeapIndex: view has no heap slot");
    return entry.HeapIndex;
}
