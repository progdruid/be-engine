#pragma once
#include <memory>
#include <string>
#include <vector>

#include "BeRenderPass.h"

class BeShader;

class CustomFullscreenEffectPass final : public BeRenderPass {
public:
    std::vector<std::string> InputTextureNames;
    std::vector<std::string> OutputTextureNames;
    BeShader* Shader;
    
public:
    explicit CustomFullscreenEffectPass();
    ~CustomFullscreenEffectPass() override;
    
    auto Initialise() -> void override;
    auto Render() -> void override;
};
