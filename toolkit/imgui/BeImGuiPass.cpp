#include "BeImGuiPass.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "BeRenderer.h"
#include "BeWindow.h"
#include "platform/dx11/BeRendererImpl.h"

BeImGuiPass::BeImGuiPass(const std::shared_ptr<BeWindow>& window)
    : _window(window) {
}

BeImGuiPass::~BeImGuiPass() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto BeImGuiPass::Initialise() -> void {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto* impl = _renderer->GetPlatformImpl();
    ImGui_ImplGlfw_InitForOther(_window->GetGlfwWindow(), true);
    ImGui_ImplDX11_Init(impl->device.Get(), impl->context.Get());
}

auto BeImGuiPass::Render() -> void {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

    if (_uiCallback) {
        _uiCallback();
    }

    ImGui::Render();

    auto* impl = _renderer->GetPlatformImpl();
    auto& backbuffer = impl->backbufferTarget;

    BeViewport vp;
    vp.Width = static_cast<float>(_renderer->GetWidth());
    vp.Height = static_cast<float>(_renderer->GetHeight());
    _renderer->GetPipeline()->SetViewport(vp);

    ID3D11RenderTargetView* rtv = backbuffer.Get();
    impl->context->OMSetRenderTargets(1, &rtv, nullptr);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    impl->context->OMSetRenderTargets(0, nullptr, nullptr);
}

auto BeImGuiPass::SetUICallback(const std::function<void()>& callback) -> void {
    _uiCallback = callback;
}
