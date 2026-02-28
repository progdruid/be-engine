#pragma once
#include <memory>
#include <string>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeShader;

class BeBackbufferPass final : public BeRenderPass {

    expose
    glm::vec3 ClearColor;
    std::weak_ptr<BeTexture> InputTexture;

    hide
    std::shared_ptr<BeShader> _backbufferShader = nullptr;
    std::shared_ptr<BeMaterial> _backbufferMaterial = nullptr;

    expose
    explicit BeBackbufferPass();
    ~BeBackbufferPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Composer Pass"; }
};
