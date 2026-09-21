#include "BeImGuiPass.h"

#include <cstdio>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <vulkan/vulkan_core.h>
#include <sen-rhi/Sen.h>
#include <sen-rhi/vulkan/SenVulkanConvert.h>

#include "BePass.h"
#include "BeRenderer.h"
#include "BeWindow.h"

BeImGuiPass::BeImGuiPass(const std::shared_ptr<BeWindow>& window)
    : _window(window) {
}

static int s_backendRefCount = 0;

BeImGuiPass::~BeImGuiPass() {
    if (!_holdsBackendRef) return;
    _holdsBackendRef = false;
    if (--s_backendRefCount > 0) return;

    Sen::WaitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto BeImGuiPass::Initialise(BeRenderer& renderer) -> void {
    _holdsBackendRef = true;
    if (s_backendRefCount++ > 0) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(_window->GetGlfwWindow(), true);

    VkFormat colorFormat = SenVk::ToFormat(renderer.GetSwapchainFormat());

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion      = VK_API_VERSION_1_3;
    init_info.Instance        = static_cast<VkInstance>(Sen::GetNativeInstance());
    init_info.PhysicalDevice  = static_cast<VkPhysicalDevice>(Sen::GetNativePhysicalDevice());
    init_info.Device          = static_cast<VkDevice>(Sen::GetNativeDevice());
    init_info.QueueFamily     = Sen::GetNativeQueueFamilyIndex();
    init_info.Queue           = static_cast<VkQueue>(Sen::GetNativeQueue());
    init_info.DescriptorPoolSize = 16;
    init_info.MinAllocationSize = 1024 * 1024;
    init_info.CheckVkResultFn = [](VkResult err) {
        if (err != VK_SUCCESS) {
            std::fprintf(stderr, "[ImGui-Vulkan] VkResult = %d\n", static_cast<int>(err));
        }
    };
    init_info.MinImageCount   = 2;
    init_info.ImageCount      = 2;
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &colorFormat,
    };

    ImGui_ImplVulkan_Init(&init_info);
}

auto BeImGuiPass::Render(BeRenderer& renderer, SenCommandList cmd) -> void {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // static int dbgFrame = 0;
    // if (dbgFrame++ == 1) {
    //     const auto& io = ImGui::GetIO();
    //     std::fprintf(stderr,
    //         "[ImGui] DisplaySize=%.0fx%.0f  FramebufferScale=%.2fx%.2f  rendererW=%u  rendererH=%u\n",
    //         io.DisplaySize.x, io.DisplaySize.y,
    //         io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y,
    //         renderer.GetWidth(), renderer.GetHeight());
    // }

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

    if (_uiCallback) {
        _uiCallback();
    }

    ImGui::Render();

    BePass pass(cmd);
    pass.AddColorTarget(renderer.GetBackbufferView(), SenLoadOp::Load);
    pass.SetViewport(renderer.GetViewport());
    pass.Begin();

    ImGui_ImplVulkan_RenderDrawData(
        ImGui::GetDrawData(),
        SenCmd::GetNativeHandle(cmd)
    );

    pass.End();
}

auto BeImGuiPass::SetUICallback(const std::function<void()>& callback) -> void {
    _uiCallback = callback;
}
