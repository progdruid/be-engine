#include <print>

#include "BeWindow.h"
#include "BeInput.h"
#include "BePass.h"
#include "sen-rhi/SenBackend.h"
#include "sen-rhi/SenShaderCompiler.h"

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

int main() {
    // todo for me (claude dont delete this comment)
    // static -> dynamic
    // semantic / location
    // render target formats
    
    auto window = std::make_shared<BeWindow>(800, 600, "be: hello triangle", BeWindowMode::Windowed);

    SenBackend::Init({ .DebugLayer = false });

    SenShaderCompiler::Launch();
    SenShaderCompiler::AddSearchPath("assets/shaders/");

    auto swapchain = SenBackend::CreateSwapchain({
        .NativeWindowHandle = window->GetGlfwWindow(),
        .Width  = uint32_t(window->GetReportedLogicalWidth()),
        .Height = uint32_t(window->GetReportedLogicalHeight()),
    });

    const SenFormat swapchainFormat = SenBackend::GetSwapchainFormat(swapchain);

    Vertex vertices[] = {
        {  0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },  // top (red)
        {  0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },  // bottom-right (green)
        { -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },  // bottom-left (blue)
    };

    auto vertexBuffer = SenBackend::CreateBuffer({
        .Usage  = SenBufferUsage::Vertex,
        .Access = SenBufferAccess::Immutable,
        .Size   = sizeof(vertices),
        .Data   = vertices,
    });

    auto vertexShader = SenBackend::CreateShader({
        .SourcePath   = "assets/shaders/triangle.hlsl",
        .FunctionName = "VSMain",
        .Stage        = SenShaderStage::Vertex,
    });

    auto pixelShader = SenBackend::CreateShader({
        .SourcePath   = "assets/shaders/triangle.hlsl",
        .FunctionName = "PSMain",
        .Stage        = SenShaderStage::Pixel,
    });

    auto pipeline = SenBackend::CreatePipeline({
        .VertexShader = vertexShader,
        .PixelShader  = pixelShader,
        .VertexLayout = {
            { .Location = 0, .Format = SenFormat::RGB32_Float,  .Offset = 0  },
            { .Location = 1, .Format = SenFormat::RGBA32_Float, .Offset = 12 },
        },
        .Topology            = SenTopology::TriangleList,
        .RenderTargetFormats = { swapchainFormat },
    });

    auto cmd = SenBackend::AllocateCommandBuffer();

    std::println("Ready. Running loop...");

    while (!window->ShouldClose()) {
        window->PollEvents();

        auto backbuffer = SenBackend::BeginFrame(swapchain);

        cmd.Begin();

        BePass pass(cmd);
        pass.AddColorTarget(backbuffer, SenLoadOp::Clear, { 0.1f, 0.1f, 0.1f, 1.0f });
        pass.SetViewport({ 0, 0, float(window->GetReportedLogicalWidth()), float(window->GetReportedLogicalHeight()), 0, 1 });
        pass.Begin();

        cmd.SetPipeline(pipeline);
        cmd.SetVertexBuffer(vertexBuffer);
        cmd.Draw(3, 0);

        pass.End();

        cmd.TransitionTextures({ { backbuffer, SenResourceState::Present } });
        cmd.End();

        SenBackend::EndFrame(swapchain, cmd);
    }

    SenBackend::WaitIdle();

    SenBackend::DestroyPipeline(pipeline);
    SenBackend::DestroyShader(pixelShader);
    SenBackend::DestroyShader(vertexShader);
    SenBackend::DestroyBuffer(vertexBuffer);
    SenBackend::DestroySwapchain(swapchain);
    SenBackend::Shutdown();
}
