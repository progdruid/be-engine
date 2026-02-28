#include "BeImGuiPass.h"

// System headers first to avoid access-modifier macro conflicts.
#import <Metal/Metal.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_metal.h"

#include "BeRenderer.h"
#include "BeWindow.h"
#include "platform/metal/BeRendererImpl.h"

BeImGuiPass::BeImGuiPass(const std::shared_ptr<BeWindow>& window)
    : _window(window) {
}

BeImGuiPass::~BeImGuiPass() {
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto BeImGuiPass::Initialise() -> void {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto* impl = _renderer->GetPlatformImpl();
    ImGui_ImplGlfw_InitForOther(_window->GetGlfwWindow(), true);
    ImGui_ImplMetal_Init(impl->device);
}

auto BeImGuiPass::Render() -> void {
    auto* impl = _renderer->GetPlatformImpl();
    if (!impl->currentDrawable) return;

    auto passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = impl->currentDrawable.texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    if (impl->currentEncoder) {
        [impl->currentEncoder endEncoding];
        impl->currentEncoder = nil;
    }

    auto encoder = [impl->currentCommandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

    ImGui_ImplMetal_NewFrame(passDescriptor);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

    if (_uiCallback) {
        _uiCallback();
    }

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), impl->currentCommandBuffer, encoder);

    [encoder endEncoding];
}

auto BeImGuiPass::SetUICallback(const std::function<void()>& callback) -> void {
    _uiCallback = callback;
}
