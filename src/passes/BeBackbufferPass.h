#pragma once
#include <memory>
#include <umbrellas/include-glm.h>
#include <string>

#include "BeRenderPass.h"

class BeMaterial;
class BeShader;

class BeBackbufferPass final : public BeRenderPass {

public:
    glm::vec3 ClearColor;

    std::string InputTextureName;
    
private:
    std::shared_ptr<BeShader> _backbufferShader = nullptr;
    std::shared_ptr<BeMaterial> _backbufferMaterial = nullptr;
    
public:
    explicit BeBackbufferPass();
    ~BeBackbufferPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Composer Pass"; }
};
