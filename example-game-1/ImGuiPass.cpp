#include "ImGuiPass.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx11.h>

#include "BeRenderer.h"

ImGuiPass::ImGuiPass(GLFWwindow* window)
    : _window(window) {
}

ImGuiPass::~ImGuiPass() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

auto ImGuiPass::Initialise() -> void {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.5f;

    ImGui_ImplGlfw_InitForOther(_window, true);
    ImGui_ImplDX11_Init(_renderer->GetDevice().Get(), _renderer->GetContext().Get());
}

auto ImGuiPass::Render() -> void {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_Once);
    ImGui::Begin("be engine example project ui controls");
    {
        if (ImGui::TreeNode("Directional Light")) {
            ImGui::ColorEdit3("Color", (float*)&DirectionalLight->Color, ImGuiColorEditFlags_DisplayRGB);
            ImGui::DragFloat("Power", &DirectionalLight->Power, 0.01f, 0.0f, 0.0f, "%.2f");
            glm::vec3 direction = DirectionalLight->Direction;
            ImGui::SliderFloat3("Direction", (float*)&direction, -1.0f, 1.0f, "%.2f");
            DirectionalLight->Direction = glm::normalize(direction);
            DirectionalLight->CalculateMatrix();
            ImGui::TreePop();
        }

        auto& pointLights = *PointLights;
        if (!pointLights.empty() && ImGui::TreeNode("Point Lights")) {
            const auto& firstLight = pointLights[0];
            auto color = firstLight.Color;
            auto power = firstLight.Power;
            auto castsShadows = firstLight.CastsShadows;
            
            ImGui::ColorEdit3("Color", (float*)&color, /*ImGuiColorEditFlags_HDR |*/ ImGuiColorEditFlags_DisplayRGB);
            ImGui::DragFloat("Power", &power, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::Checkbox("Cast Shadows", &castsShadows);

            for (auto& pointLight : pointLights) {
                pointLight.Color = color;
                pointLight.Power = power;
                pointLight.CastsShadows = castsShadows;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Terrain")) {
            auto noiseResolution = TerrainMaterial->GetFloat("NoiseResolution");
            auto speed = TerrainMaterial->GetFloat("Speed");
            ImGui::DragFloat("Noise Resolution", &noiseResolution, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Speed ", &speed, 0.01f, 0.0f, 0.0f, "%.2f");
            TerrainMaterial->SetFloat("NoiseResolution", noiseResolution);
            TerrainMaterial->SetFloat("Speed", speed);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Living Cube")) {
            auto diffuse = LivingCubeMaterial->GetFloat3("DiffuseColor");
            auto specular = LivingCubeMaterial->GetFloat3("SpecularColor");
            auto shininess = LivingCubeMaterial->GetFloat("Shininess");
            auto tesselationLevel = LivingCubeMaterial->GetFloat("TessellationLevel");
            auto displacementStrength = LivingCubeMaterial->GetFloat("DisplacementStrength");
            auto speed = LivingCubeMaterial->GetFloat("AnimationSpeed");
            auto noiseFrequency = LivingCubeMaterial->GetFloat("NoiseFrequency");

            ImGui::ColorEdit3("Diffuse Color", (float*)&diffuse, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit3("Specular Color", (float*)&specular, ImGuiColorEditFlags_DisplayRGB);
            ImGui::DragFloat("Shininess", &shininess, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Tessellation Level", &tesselationLevel, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Displacement Strength", &displacementStrength, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Animation Speed", &speed, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Noise Frequency", &noiseFrequency, 0.01f, 0.0f, 0.0f, "%.2f");

            LivingCubeMaterial->SetFloat3("DiffuseColor", diffuse);
            LivingCubeMaterial->SetFloat3("SpecularColor", specular);
            LivingCubeMaterial->SetFloat("Shininess", shininess);
            LivingCubeMaterial->SetFloat("TessellationLevel", tesselationLevel);
            LivingCubeMaterial->SetFloat("DisplacementStrength", displacementStrength);
            LivingCubeMaterial->SetFloat("AnimationSpeed", speed);
            LivingCubeMaterial->SetFloat("NoiseFrequency", noiseFrequency);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Bloom")) {
            const auto material = BloomMaterial.lock();

            auto threshold = material->GetFloat("Threshold");
            auto intensity = material->GetFloat("Intensity");
            auto knee = material->GetFloat("Knee");

            ImGui::DragFloat("Threshold", &threshold, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragFloat("Knee", &knee, 0.01f, 0.0f, 0.0f, "%.2f");
            
            material->SetFloat("Threshold", threshold);
            material->SetFloat("Intensity", intensity);
            material->SetFloat("Knee", knee);

            material->UpdateGPUBuffers(_renderer->GetContext());
            
            ImGui::TreePop();
        }
    }
    ImGui::End();

    ImGui::Render();
    
    auto context = _renderer->GetContext();
    auto backbuffer = _renderer->GetBackbufferTarget();
    
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(_renderer->GetWidth());
    viewport.Height = static_cast<float>(_renderer->GetHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    context->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView* rtv = backbuffer.Get();
    context->OMSetRenderTargets(1, &rtv, nullptr);
    
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplDX11_RenderDrawData(drawData);

    context->OMSetRenderTargets(0, nullptr, nullptr);
}
