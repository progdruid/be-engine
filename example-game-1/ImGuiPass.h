#pragma once

#include "BeBuffers.h"
#include "passes/BeRenderPass.h"

struct GLFWwindow;

class ImGuiPass final : public BeRenderPass {
    expose
    BeDirectionalLight* DirectionalLight;
    std::vector<BePointLight>* PointLights;
    std::shared_ptr<BeMaterial> TerrainMaterial;
    std::shared_ptr<BeMaterial> LivingCubeMaterial;

    hide
    GLFWwindow* _window = nullptr;
    
    expose
    explicit ImGuiPass(GLFWwindow* window);
    ~ImGuiPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "ImGui Pass"; }
};
