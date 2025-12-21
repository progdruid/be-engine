#pragma once
#include <d3d11.h>
#include <memory>
#include <span>
#include <string>
#include <umbrellas/include-glm.h>

#include "BeRenderPass.h"

struct BePointLight;
struct BeDirectionalLight;

class BeShadowPass final : public BeRenderPass {

    expose
    std::weak_ptr<BeDirectionalLight> DirectionalLight;
    std::span<BePointLight> PointLights;

    hide
    ComPtr<ID3D11Buffer> _objectBuffer;
    
    expose
    explicit BeShadowPass() = default;
    ~BeShadowPass() override = default;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Shadow Pass"; }

    hide
    auto RenderDirectionalShadows() -> void;
    auto RenderPointLightShadows(const BePointLight& pointLight) -> void;

    auto CalculatePointLightFaceViewProjection(const BePointLight& pointLight, int faceIndex) -> glm::mat4;
};
