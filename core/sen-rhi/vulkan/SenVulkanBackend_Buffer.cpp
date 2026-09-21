#include "SenVulkanBackend.h"

#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateBuffer(const SenBufferDesc& desc) -> SenBuffer {
    auto entry = SenVulkanBufferEntry();
    entry.Memory = desc.Memory;
    entry.Size   = desc.Size;

    const VkBufferCreateInfo bufferInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = desc.Size,
        .usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                     | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                     | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                     | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                     | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                     | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (desc.Memory == SenMemory::Upload) {
        // Device-local host-visible (ReBAR) where available so BDA-dereferenced reads hit VRAM,
        // falling back to system RAM otherwise. CPU writes via memcpy, GPU reads after submit.
        VmaAllocationCreateInfo allocInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VmaAllocationInfo allocResult;
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, &allocResult);
        be_assert(result == VK_SUCCESS, "Failed to create upload buffer!");

        entry.MappedPtr = allocResult.pMappedData;
    } else {
        // Device-local VRAM — GPU reads fastest from here, CPU writes arrive through a copy.
        VmaAllocationCreateInfo allocInfo {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, nullptr);
        be_assert(result == VK_SUCCESS, "Failed to create device-local buffer!");
    }

    const VkBufferDeviceAddressInfo addressInfo {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = entry.Buffer,
    };
    entry.GpuAddress = vkGetBufferDeviceAddress(_device, &addressInfo);

    return _buffers.Create(std::move(entry));
}

auto SenVulkanBackend::DestroyBuffer(SenBuffer handle) -> void {
    if (!_buffers.Contains(handle)) {
        return;
    }

    auto& entry = _buffers.Get(handle);
    vmaDestroyBuffer(_allocator, entry.Buffer, entry.Allocation);
    _buffers.Destroy(handle);
}

auto SenVulkanBackend::LookupBuffer(SenBuffer handle) -> SenVulkanBufferEntry& {
    return _buffers.Get(handle);
}

auto SenVulkanBackend::GetBufferAddress(SenBuffer handle) -> SenGpuAddress {
    return { _buffers.Get(handle).GpuAddress };
}

auto SenVulkanBackend::GetBufferMemory(SenBuffer handle) -> SenMemory {
    return _buffers.Get(handle).Memory;
}

auto SenVulkanBackend::GetBufferPointer(SenBuffer handle) -> void* {
    auto& entry = _buffers.Get(handle);
    be_assert(entry.Memory == SenMemory::Upload, "GetBufferPointer: buffer is not Upload memory");
    return entry.MappedPtr;
}

