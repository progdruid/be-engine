
#include <print>

#include "BeWindow.h"
#include "sen-rhi/SenBackend.h"

void main () {
    
    auto window = std::make_shared<BeWindow>(100, 100, "be: example sakura", BeWindowMode::Windowed);
    
    SenBackend::Init({
        .DebugLayer = false
    });
    
    auto swapchain = SenBackend::CreateSwapchain({
        .NativeWindowHandle = window->GetHwnd(),
        .Width =  uint32_t(window->GetWidth()),
        .Height = uint32_t(window->GetHeight()),
    });
    
    
    std::println("Started swapchain!");
    
    SenBackend::DestroySwapchain(swapchain);
    SenBackend::Shutdown();
    
    return;
}
