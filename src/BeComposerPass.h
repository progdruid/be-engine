#pragma once
#include <memory>
#include <glm.hpp>

#include "BeRenderPass.h"

class BeShader;

class BeComposerPass : public BeRenderPass {

public:
    glm::vec3 ClearColor;
    
private:
    std::unique_ptr<BeShader> _fullscreenShader = nullptr;
    
public:
    explicit BeComposerPass() = default;
    ~BeComposerPass() override = default;

    [[nodiscard]] auto GetInputResources() const -> std::vector<std::string> override;
    [[nodiscard]] auto GetOutputResources() const -> std::vector<std::string> override;
    auto Initialise() -> void override;
    auto Render() -> void override;
};
