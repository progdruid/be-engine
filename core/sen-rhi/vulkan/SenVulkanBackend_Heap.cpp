#include "SenVulkanBackend.h"

#include <sen-rhi/vulkan/SenVulkanConvert.h>
#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::InitBindlessHeap() -> void {
    VkPhysicalDeviceVulkan12Properties props12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 props2 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &props12,
    };
    vkGetPhysicalDeviceProperties2(_physicalDevice, &props2);

    _heapCapacity[size_t(SenHeapBinding::Texture2D)]        = 4096;
    _heapCapacity[size_t(SenHeapBinding::Texture2DArray)]   = 256;
    _heapCapacity[size_t(SenHeapBinding::TextureCube)]      = 256;
    _heapCapacity[size_t(SenHeapBinding::TextureCubeArray)] = 64;
    _heapCapacity[size_t(SenHeapBinding::StorageTexture2D)] = 256;
    _heapCapacity[size_t(SenHeapBinding::Sampler)]          = 256;

    const uint32_t sampledTotal =
        _heapCapacity[size_t(SenHeapBinding::Texture2D)] +
        _heapCapacity[size_t(SenHeapBinding::Texture2DArray)] +
        _heapCapacity[size_t(SenHeapBinding::TextureCube)] +
        _heapCapacity[size_t(SenHeapBinding::TextureCubeArray)];
    be_assert(sampledTotal <= props12.maxDescriptorSetUpdateAfterBindSampledImages,
              "Bindless heap: sampled-image capacity exceeds device limit");
    be_assert(_heapCapacity[size_t(SenHeapBinding::StorageTexture2D)] <= props12.maxDescriptorSetUpdateAfterBindStorageImages,
              "Bindless heap: storage-image capacity exceeds device limit");
    be_assert(_heapCapacity[size_t(SenHeapBinding::Sampler)] <= props12.maxDescriptorSetUpdateAfterBindSamplers,
              "Bindless heap: sampler capacity exceeds device limit");

    std::array<VkDescriptorSetLayoutBinding, size_t(SenHeapBinding::Count)> bindings {};
    std::array<VkDescriptorBindingFlags, size_t(SenHeapBinding::Count)>     bindingFlags {};
    for (uint32_t i = 0; i < uint32_t(SenHeapBinding::Count); ++i) {
        bindings[i] = VkDescriptorSetLayoutBinding {
            .binding         = i,
            .descriptorType  = Sen::Vulkan::ToHeapDescriptorType(SenHeapBinding(i)),
            .descriptorCount = _heapCapacity[i],
            .stageFlags      = VK_SHADER_STAGE_ALL,
        };
        bindingFlags[i] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
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
    be_assert(result == VK_SUCCESS, "Bindless heap: failed to create set layout");

    std::array<VkDescriptorPoolSize, 3> poolSizes {
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledTotal },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _heapCapacity[size_t(SenHeapBinding::StorageTexture2D)] },
        VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER,       _heapCapacity[size_t(SenHeapBinding::Sampler)] },
    };
    VkDescriptorPoolCreateInfo poolInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets       = 1,
        .poolSizeCount = uint32_t(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    };
    result = vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_bindlessPool);
    be_assert(result == VK_SUCCESS, "Bindless heap: failed to create pool");

    VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = _bindlessPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &_bindlessLayout,
    };
    result = vkAllocateDescriptorSets(_device, &allocInfo, &_bindlessSet);
    be_assert(result == VK_SUCCESS, "Bindless heap: failed to allocate set");
}

auto SenVulkanBackend::ShutdownBindlessHeap() -> void {
    if (_bindlessPool)   { vkDestroyDescriptorPool(_device, _bindlessPool, nullptr); _bindlessPool = VK_NULL_HANDLE; }
    if (_bindlessLayout) { vkDestroyDescriptorSetLayout(_device, _bindlessLayout, nullptr); _bindlessLayout = VK_NULL_HANDLE; }
    _bindlessSet = VK_NULL_HANDLE;
    for (auto& next : _heapNext) { next = 0; }
    for (auto& free : _heapFree) { free.clear(); }
}

auto SenVulkanBackend::HeapAllocSlot(SenHeapBinding binding) -> uint32_t {
    const size_t b = size_t(binding);
    if (!_heapFree[b].empty()) {
        const uint32_t slot = _heapFree[b].back();
        _heapFree[b].pop_back();
        return slot;
    }
    const uint32_t slot = _heapNext[b]++;
    be_assert(slot < _heapCapacity[b], "Bindless heap: binding out of slots");
    return slot;
}

auto SenVulkanBackend::HeapRegisterTexture(SenVulkanTextureEntry& entry, VkImageView view, VkImageViewType viewType) -> void {
    const SenHeapBinding binding = Sen::Vulkan::ToHeapBinding(viewType);
    const uint32_t slot = HeapAllocSlot(binding);

    VkDescriptorImageInfo imageInfo {
        .imageView   = view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _bindlessSet,
        .dstBinding      = uint32_t(binding),
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

    entry.HeapBinding = uint32_t(binding);
    entry.HeapIndex   = slot;
}

auto SenVulkanBackend::HeapRegisterSampler(SenVulkanSamplerEntry& entry) -> void {
    const uint32_t slot = HeapAllocSlot(SenHeapBinding::Sampler);

    VkDescriptorImageInfo imageInfo {
        .sampler = entry.Sampler,
    };
    VkWriteDescriptorSet write {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = _bindlessSet,
        .dstBinding      = uint32_t(SenHeapBinding::Sampler),
        .dstArrayElement = slot,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = &imageInfo,
    };
    vkUpdateDescriptorSets(_device, 1, &write, 0, nullptr);

    entry.HeapIndex = slot;
}

auto SenVulkanBackend::HeapReleaseTexture(SenVulkanTextureEntry& entry) -> void {
    if (entry.HeapIndex == UINT32_MAX) { return; }
    _heapFree[entry.HeapBinding].push_back(entry.HeapIndex);
    entry.HeapBinding = UINT32_MAX;
    entry.HeapIndex   = UINT32_MAX;
}

auto SenVulkanBackend::HeapReleaseSampler(SenVulkanSamplerEntry& entry) -> void {
    if (entry.HeapIndex == UINT32_MAX) { return; }
    _heapFree[size_t(SenHeapBinding::Sampler)].push_back(entry.HeapIndex);
    entry.HeapIndex = UINT32_MAX;
}

auto SenVulkanBackend::GetTextureHeapIndex(SenTexture handle) -> uint32_t {
    auto& entry = _textures.at(handle.ID);
    be_assert(entry.HeapIndex != UINT32_MAX, "GetTextureHeapIndex: texture has no heap slot (not a shader resource?)");
    return entry.HeapIndex;
}

auto SenVulkanBackend::GetSamplerHeapIndex(SenSampler handle) -> uint32_t {
    auto& entry = _samplers.at(handle.ID);
    be_assert(entry.HeapIndex != UINT32_MAX, "GetSamplerHeapIndex: sampler has no heap slot");
    return entry.HeapIndex;
}
