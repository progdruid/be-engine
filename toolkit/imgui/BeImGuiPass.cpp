#include "BeImGuiPass.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "BeRenderer.h"
#include "BeWindow.h"
#include <sen-rhi/SenBackend.h>

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
    ImGui_ImplDX11_Init(
        static_cast<ID3D11Device*>(SenBackend::GetNativeDevice()),
        static_cast<ID3D11DeviceContext*>(SenBackend::GetNativeContext())
    );
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

    // Begin pass with backbuffer (Load to preserve existing content)
    auto& cmd = _renderer->GetCommandBuffer();
    cmd.BeginPass({
        .ColorAttachments = {
            { _renderer->GetBackbufferTexture(), 0, -1, SenLoadOp::Load },
        },
        .Viewport = _renderer->GetViewport(),
    });

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    cmd.EndPass();
}

auto BeImGuiPass::SetUICallback(const std::function<void()>& callback) -> void {
    _uiCallback = callback;
}
