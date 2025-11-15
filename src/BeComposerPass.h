#pragma once
#include <memory>
#include <glm.hpp>
#include <string>

#include "BeRenderPass.h"

class BeShader;

class BeComposerPass final : public BeRenderPass {

public:
    glm::vec3 ClearColor;

    std::string InputDepthTextureName;
    std::string InputTexture0Name;
    std::string InputTexture1Name;
    std::string InputTexture2Name;
    std::string InputLightTextureName;
    
private:
    std::shared_ptr<BeShader> _composerShader = nullptr;
    
public:
    explicit BeComposerPass();
    ~BeComposerPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Composer Pass"; }
};
