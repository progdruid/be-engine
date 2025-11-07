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
    std::string InputPointLightsName;

private:
    std::unique_ptr<BeShader> _directionalShadowShader;
    std::unique_ptr<BeShader> _pointShadowShader;

    ComPtr<ID3D11Buffer> _shadowConstantBuffer;
    
public:
    explicit ShadowPass() = default;
    ~ShadowPass() override = default;
    
    auto Initialise() -> void override;
    auto Render() -> void override;

private:
    auto RenderDirectionalShadows() -> void;
    auto RenderPointLightShadows(const BePointLight& pointLight) -> void;

    auto CalculatePointLightFaceViewProjection(const BePointLight& pointLight, int faceIndex) -> glm::mat4;
};
