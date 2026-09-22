#include "SenVulkanInterop.h"

#include "SenVulkanState.h"


namespace SenVulkanInterop {

auto GetInstance() -> VkInstance { return SenVk::_instance; }
auto GetPhysicalDevice() -> VkPhysicalDevice { return SenVk::_physicalDevice; }
auto GetDevice() -> VkDevice { return SenVk::_device; }
auto GetQueue() -> VkQueue { return SenVk::_queues[uint32_t(SenQueue::Graphics)].Queue; }
auto GetQueueFamilyIndex() -> uint32_t { return SenVk::_queues[uint32_t(SenQueue::Graphics)].FamilyIndex; }
auto GetCommandBuffer(SenCommandList list) -> VkCommandBuffer { return SenVk::LookupCommandList(list).Cmd; }

}
