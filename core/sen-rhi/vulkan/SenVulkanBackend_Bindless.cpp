#include "SenVulkanState.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

using namespace SenVk;

// must match the bindings be-bindless-tables.hlsl declares its arrays at
constexpr uint32_t TextureBinding = 0;
constexpr uint32_t StorageBinding = 1;
constexpr uint32_t SamplerBinding = 2;

auto SenVk::InitBindless() -> void {
    VkPhysicalDeviceVulkan12Properties props12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 props2 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &props12,
    };
    vkGetPhysicalDeviceProperties2(_physicalDevice, &props2);

    be_assert(BindlessTextureSlots <= props12.maxDescriptorSetUpdateAfterBindSampledImages, "Bindless: sampled-image capacity exceeds device limit");
    be_assert(BindlessStorageSlots <= props12.maxDescriptorSetUpdateAfterBindStorageImages, "Bindless: storage-image capacity exceeds device limit");
    be_assert(BindlessSamplerSlots <= props12.maxDescriptorSetUpdateAfterBindSamplers,      "Bindless: sampler capacity exceeds device limit");

    const std::array bindings {
        VkDescriptorSetLayoutBinding {
            .binding         = TextureBinding,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = BindlessTextureSlots,
            .stageFlags      = VK_SHADER_STAGE_ALL,
        },
        VkDescriptorSetLayoutBinding {
            .binding         = StorageBinding,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = BindlessStorageSlots,
            .stageFlags      = VK_SHADER_STAGE_ALL,
        },
        VkDescriptorSetLayoutBinding {
            .binding         = SamplerBinding,
            .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = BindlessSamplerSlots,
            .stageFlags      = VK_SHADER_STAGE_ALL,
        },
    };

    std::array<VkDescriptorBindingFlags, bindings.size()> bindingFlags {};
    for (auto& flags : bindingFlags) {
        flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = uint32_t(bindingFlags.size()),
        .pBindingFlags = bindingFlags.data(),
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = &flagsInfo,
        .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = uint32_t(bindings.size()),
        .pBindings    = bindings.data(),
    };
    VkResult result = vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_bindlessLayout);
    be_assert(result == VK_SUCCESS, "Bindless: failed to create set layout");

    const std::array poolSizes {
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, BindlessTextureSlots },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, BindlessStorageSlots },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER,       BindlessSamplerSlots },
    };
    VkDescriptorPoolCreateInfo poolInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets       = 1,
        .poolSizeCount = uint32_t(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    };
    result = vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_bindlessPool);
    be_assert(result == VK_SUCCESS, "Bindless: failed to create pool");

    VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = _bindlessPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &_bindlessLayout,
    };
    result = vkAllocateDescriptorSets(_device, &allocInfo, &_bindlessSet);
    be_assert(result == VK_SUCCESS, "Bindless: failed to allocate set");
}

auto SenVk::ShutdownBindless() -> void {
    if (_bindlessPool)   { vkDestroyDescriptorPool(_device, _bindlessPool, nullptr); _bindlessPool = VK_NULL_HANDLE; }
    if (_bindlessLayout) { vkDestroyDescriptorSetLayout(_device, _bindlessLayout, nullptr); _bindlessLayout = VK_NULL_HANDLE; }
    _bindlessSet = VK_NULL_HANDLE;
}

auto Sen::PublishTextureBindless(uint32_t slot, SenView view) -> void {
    be_assert(slot < BindlessTextureSlots, "PublishTextureBindless: slot out of range", slot);

    VkDescriptorImageInfo imageInfo {
        .imageView   = _views.Get(view).View,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _bindlessSet,
        .dstBinding      = TextureBinding,
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
}

auto Sen::PublishStorageBindless(uint32_t slot, SenView view) -> void {
    be_assert(slot < BindlessStorageSlots, "PublishStorageBindless: slot out of range", slot);

    VkDescriptorImageInfo imageInfo {
        .imageView   = _views.Get(view).View,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _bindlessSet,
        .dstBinding      = StorageBinding,
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
}

auto Sen::PublishSamplerBindless(uint32_t slot, SenSampler sampler) -> void {
    be_assert(slot < BindlessSamplerSlots, "PublishSamplerBindless: slot out of range", slot);

    VkDescriptorImageInfo imageInfo {
        .sampler = _samplers.Get(sampler).Sampler,
    };
    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _bindlessSet,
        .dstBinding      = SamplerBinding,
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);
}
