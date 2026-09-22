#include "SenVulkanState.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

using namespace SenVk;

auto Sen::CreateSampler(const SenSamplerDesc& desc) -> SenSampler {
    auto entry = SenVulkanSamplerEntry();

    VkSamplerCreateInfo samplerInfo {
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter        = SenVk::ToFilter(desc.Filter),
        .minFilter        = SenVk::ToFilter(desc.Filter),
        .mipmapMode       = SenVk::ToMipmapMode(desc.Filter),
        .addressModeU     = SenVk::ToAddressMode(desc.Address),
        .addressModeV     = SenVk::ToAddressMode(desc.Address),
        .addressModeW     = SenVk::ToAddressMode(desc.Address),
        .anisotropyEnable = (desc.Filter == SenFilter::Anisotropic) ? VK_TRUE : VK_FALSE,
        .maxAnisotropy    = (desc.Filter == SenFilter::Anisotropic) ? 16.f : 1.f,
        .compareEnable    = desc.Comparison ? VK_TRUE : VK_FALSE,
        .compareOp        = desc.Comparison ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_NEVER,
        .minLod           = 0.f,
        .maxLod           = VK_LOD_CLAMP_NONE,
    };

    VkResult result = vkCreateSampler(_device, &samplerInfo, nullptr, &entry.Sampler);
    be_assert(result == VK_SUCCESS, "Failed to create sampler!");

    return _samplers.Create(std::move(entry));
}

auto Sen::DestroySampler(SenSampler handle) -> void {
    if (!_samplers.Contains(handle)) {
        return;
    }

    auto& entry = _samplers.Get(handle);
    vkDestroySampler(_device, entry.Sampler, nullptr);
    _samplers.Destroy(handle);
}

auto SenVk::LookupSampler(SenSampler handle) -> SenVulkanSamplerEntry& {
    return _samplers.Get(handle);
}
