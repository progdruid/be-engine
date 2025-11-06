#pragma once
#include <memory>
#include <string>
#include <vec3.hpp>

#include "BeBuffers.h"
#include "BeRenderPass.h"
#include "BeShader.h"

class ShadowPass final : public BeRenderPass {

public:
    std::string InputDirectionalLightName;

private:
    std::unique_ptr<BeShader> _shadowShader;
    
    ComPtr<ID3D11Buffer> _shadowPassConstantBuffer;
    
public:
    explicit ShadowPass() = default;
    ~ShadowPass() override = default;
    
    auto Initialise() -> void override;
    auto Render() -> void override;
};
