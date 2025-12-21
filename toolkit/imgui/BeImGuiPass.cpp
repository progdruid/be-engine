#include "BeImGuiPass.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "BeRenderer.h"
#include "BeWindow.h"

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

    ImGui_ImplGlfw_InitForOther(_window->GetGlfwWindow(), true);
    ImGui_ImplDX11_Init(_renderer->GetDevice().Get(), _renderer->GetContext().Get());
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

    const auto context = _renderer->GetContext();
    const auto backbuffer = _renderer->GetBackbufferTarget();

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(_renderer->GetWidth());
    viewport.Height = static_cast<float>(_renderer->GetHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView* rtv = backbuffer.Get();
    context->OMSetRenderTargets(1, &rtv, nullptr);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    context->OMSetRenderTargets(0, nullptr, nullptr);
}

auto BeImGuiPass::SetUICallback(const std::function<void()>& callback) -> void {
    _uiCallback = callback;
}
