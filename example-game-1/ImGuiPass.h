#pragma once

#include <memory>
#include <vector>
#include "passes/BeRenderPass.h"

struct BePointLight;
struct BeDirectionalLight;
class BeMaterial;
struct GLFWwindow;

class ImGuiPass final : public BeRenderPass {
    expose
    BeDirectionalLight* DirectionalLight;
    std::vector<BePointLight>* PointLights;
    std::shared_ptr<BeMaterial> TerrainMaterial;
    std::shared_ptr<BeMaterial> LivingCubeMaterial;
    std::weak_ptr<BeMaterial> BloomMaterial;

    hide
    GLFWwindow* _window = nullptr;
    
    expose
    explicit ImGuiPass(GLFWwindow* window);
    ~ImGuiPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "ImGui Pass"; }
};

