#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

// Khronos validation layer wiring. Every entry point is a no-op in release builds.
namespace Sen::Vulkan::Validation {

// Before vkCreateInstance: if the validation layer is discoverable, enable it + the debug-utils
// extension (appending to the given vectors) and point the loader at the vendored layer copied next
// to the executable. Returns a messenger create-info to chain into VkInstanceCreateInfo.pNext, or
// nullptr when validation is off. The returned pointer stays valid until the next call.
auto ConfigureInstance(std::vector<const char*>& layers, std::vector<const char*>& extensions) -> const void*;

// After vkCreateInstance: create the persistent debug messenger (if validation was enabled).
auto CreateMessenger(VkInstance instance) -> void;

// During shutdown: destroy the messenger (safe to call unconditionally).
auto DestroyMessenger(VkInstance instance) -> void;

}
