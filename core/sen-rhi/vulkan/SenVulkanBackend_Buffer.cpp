#include "SenVulkanBackend.h"

#include <umbrellas/include-libassert.h>

auto SenVulkanBackend::CreateBuffer(const SenBufferDesc& desc) -> SenBuffer {
    const SenBuffer handle { _nextBufferId++ };
    auto& entry = _buffers[handle.ID];
    entry.Access = desc.Access;
    entry.Size   = desc.Size;

    VkBufferUsageFlags usageFlags = 0;
    switch (desc.Usage) {
        case SenBufferUsage::Vertex:   usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;  break;
        case SenBufferUsage::Index:    usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;   break;
        case SenBufferUsage::Constant: usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
    }

    VkBufferCreateInfo bufferInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = desc.Size,
        .usage       = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (desc.Access == SenBufferAccess::Dynamic) {
        // Host-visible, persistently mapped — CPU writes via memcpy, GPU reads after submit.
        VmaAllocationCreateInfo allocInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VmaAllocationInfo allocResult;
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, &allocResult);
        be_assert(result == VK_SUCCESS, "Failed to create dynamic buffer!");

        entry.MappedPtr = allocResult.pMappedData;

        if (desc.Data) {
            memcpy(entry.MappedPtr, desc.Data, desc.Size);
        }
    } else {
        // Device-local VRAM — GPU reads fastest from here.
        // CPU uploads go through a temporary staging buffer.
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocInfo {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &entry.Buffer, &entry.Allocation, nullptr);
        be_assert(result == VK_SUCCESS, "Failed to create device-local buffer!");

        if (desc.Data) {
            UploadToDeviceBuffer(entry.Buffer, desc.Data, desc.Size, 0);
        }
    }

    return handle;
}

auto SenVulkanBackend::RetireBuffer(SenBuffer handle) -> void {
    if (_buffers.contains(handle.ID)) {
        _retirements.push_back({ SenVulkanRetirementNote::Kind::Buffer, handle.ID, _timelineValue + 1 });
    }
}

auto SenVulkanBackend::LookupBuffer(SenBuffer handle) -> SenVulkanBufferEntry& {
    return _buffers.at(handle.ID);
}

auto SenVulkanBackend::WriteBuffer(SenBuffer handle, const void* data, uint32_t size, uint32_t dstOffset) -> void {
    auto& entry = _buffers.at(handle.ID);
    be_assert(entry.Access != SenBufferAccess::Immutable, "Cannot write to an Immutable buffer");
    be_assert(dstOffset + size <= entry.Size, "WriteBuffer: write runs past the end of the buffer");

    if (entry.Access == SenBufferAccess::Dynamic) {
        memcpy(static_cast<uint8_t*>(entry.MappedPtr) + dstOffset, data, size);
    } else {
        UploadToDeviceBuffer(entry.Buffer, data, size, dstOffset);
    }
}

// For Immutable/Default buffers the destination is device-local (GPU VRAM), which the CPU
// cannot write to directly. So we:
//   1. Create a temporary host-visible "staging" buffer and memcpy the data into it.
//   2. Record a one-time command buffer that does vkCmdCopyBuffer staging → dst.
//   3. Submit it, wait (fence), then destroy the staging buffer.
// This is a synchronous stall, but only happens at resource-creation time (or rarely for Default).
auto SenVulkanBackend::UploadToDeviceBuffer(VkBuffer dst, const void* data, uint32_t size, uint32_t dstOffset) -> void {
    VkBufferCreateInfo stagingInfo {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo stagingAllocInfo {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocResult;
    VkResult result = vmaCreateBuffer(_allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocResult);
    be_assert(result == VK_SUCCESS, "Failed to create staging buffer!");

    memcpy(stagingAllocResult.pMappedData, data, size);

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
    VkBufferCopy copyRegion { .dstOffset = dstOffset, .size = size };
    vkCmdCopyBuffer(cmd, stagingBuffer, dst, 1, &copyRegion);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(_device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    vkQueueSubmit(_queue, 1, &submitInfo, fence);
    vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(_device, fence, nullptr);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
    vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);
}
