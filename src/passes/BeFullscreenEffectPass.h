#pragma once
#include <memory>
#include <string>
#include <vector>

#include "BeRenderPass.h"

class BeShader;

class BeFullscreenEffectPass final : public BeRenderPass {
public:
    std::vector<std::string> InputTextureNames;
    std::vector<std::string> OutputTextureNames;
    BeShader* Shader;
    
public:
    explicit BeFullscreenEffectPass();
    ~BeFullscreenEffectPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Effect Pass"; }
};
