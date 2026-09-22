#include "SenVulkanSurface.h"

#include <cstring>
#include <umbrellas/include-libassert.h>

// mirrored API. no vulkan_xlib.h and friends included, 
// because they drag in conflicting includes and bloat the translation unit. 
// their layouts are frozen by KHR extensions.
namespace {

struct SenVulkanXlibSurfaceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* dpy;
    unsigned long window;
};

struct SenVulkanWaylandSurfaceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* display;
    void* surface;
};

struct SenVulkanWin32SurfaceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* hinstance;
    void* hwnd;
};

struct SenVulkanMetalSurfaceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    const void* layer;
};

using PFN_SenVulkanCreateXlibSurface = VkResult (VKAPI_PTR*)(
    VkInstance, const SenVulkanXlibSurfaceCreateInfo*, const VkAllocationCallbacks*, VkSurfaceKHR*);
using PFN_SenVulkanCreateWaylandSurface = VkResult (VKAPI_PTR*)(
    VkInstance, const SenVulkanWaylandSurfaceCreateInfo*, const VkAllocationCallbacks*, VkSurfaceKHR*);
using PFN_SenVulkanCreateWin32Surface = VkResult (VKAPI_PTR*)(
    VkInstance, const SenVulkanWin32SurfaceCreateInfo*, const VkAllocationCallbacks*, VkSurfaceKHR*);
using PFN_SenVulkanCreateMetalSurface = VkResult (VKAPI_PTR*)(
    VkInstance, const SenVulkanMetalSurfaceCreateInfo*, const VkAllocationCallbacks*, VkSurfaceKHR*);

constexpr const char* SurfaceExtension = "VK_KHR_surface";
constexpr const char* PlatformExtensions[] = {
    "VK_KHR_xlib_surface",
    "VK_KHR_wayland_surface",
    "VK_KHR_win32_surface",
    "VK_EXT_metal_surface",
};

}

auto SenVulkanSurface::ConfigureForInstance(std::vector<const char*>& extensions) -> void {
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());

    const auto isAvailable = [&available](const char* name) {
        for (const auto& extension : available) {
            if (std::strcmp(extension.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    };

    be_assert(isAvailable(SurfaceExtension), "Vulkan: VK_KHR_surface is not available");
    extensions.push_back(SurfaceExtension);

    for (const char* name : PlatformExtensions) {
        if (isAvailable(name)) {
            extensions.push_back(name);
        }
    }
}

auto SenVulkanSurface::Create(VkInstance instance, const SenNativeWindow& window) -> VkSurfaceKHR {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    switch (window.System) {
        case SenWindowSystem::Xlib: {
            const auto create = reinterpret_cast<PFN_SenVulkanCreateXlibSurface>(
                vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR"));
            be_assert(create != nullptr, "Vulkan: vkCreateXlibSurfaceKHR is not available");
            const SenVulkanXlibSurfaceCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .dpy = window.Display,
                .window = reinterpret_cast<unsigned long>(window.Window),
            };
            result = create(instance, &info, nullptr, &surface);
            break;
        }
        case SenWindowSystem::Wayland: {
            const auto create = reinterpret_cast<PFN_SenVulkanCreateWaylandSurface>(
                vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR"));
            be_assert(create != nullptr, "Vulkan: vkCreateWaylandSurfaceKHR is not available");
            const SenVulkanWaylandSurfaceCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .display = window.Display,
                .surface = window.Window,
            };
            result = create(instance, &info, nullptr, &surface);
            break;
        }
        case SenWindowSystem::Win32: {
            const auto create = reinterpret_cast<PFN_SenVulkanCreateWin32Surface>(
                vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
            be_assert(create != nullptr, "Vulkan: vkCreateWin32SurfaceKHR is not available");
            const SenVulkanWin32SurfaceCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .hinstance = nullptr,
                .hwnd = window.Window,
            };
            result = create(instance, &info, nullptr, &surface);
            break;
        }
        case SenWindowSystem::Metal: {
            const auto create = reinterpret_cast<PFN_SenVulkanCreateMetalSurface>(
                vkGetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT"));
            be_assert(create != nullptr, "Vulkan: vkCreateMetalSurfaceEXT is not available");
            const SenVulkanMetalSurfaceCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = 0,
                .layer = window.Window,
            };
            result = create(instance, &info, nullptr, &surface);
            break;
        }
    }

    be_assert(result == VK_SUCCESS, "Vulkan: failed to create surface", int(window.System));
    return surface;
}
