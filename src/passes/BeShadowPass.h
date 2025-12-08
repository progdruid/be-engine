#pragma once
#include <memory>
#include <string>

#include "BeBuffers.h"
#include "BeRenderPass.h"
#include "BeShader.h"

class BeShadowPass final : public BeRenderPass {

public:
    BeDirectionalLight* DirectionalLight;
    std::vector<BePointLight>* PointLights;

private:
    std::shared_ptr<BeShader> _directionalShadowShader;
    std::shared_ptr<BeShader> _pointShadowShader;

    ComPtr<ID3D11Buffer> _objectBuffer;
    
public:
    explicit BeShadowPass() = default;
    ~BeShadowPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Shadow Pass"; }

private:
    auto RenderDirectionalShadows() -> void;
    auto RenderPointLightShadows(const BePointLight& pointLight) -> void;

    auto CalculatePointLightFaceViewProjection(const BePointLight& pointLight, int faceIndex) -> glm::mat4;
};
