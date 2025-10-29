#pragma once
#include "BeBuffers.h"
#include "BeRenderPass.h"

class BeRenderer;

class BeLightingPass final : public BeRenderPass {
public:
    DirectionalLightData DirectionalLightData;
    std::vector<PointLightData> PointLights;

private:
    
    ComPtr<ID3D11BlendState> _lightingBlendState;

    ComPtr<ID3D11Buffer> _directionalLightBuffer;
    ComPtr<ID3D11Buffer> _pointLightBuffer;
    //ComPtr<ID3D11Buffer> _spotLightBuffer;

    std::unique_ptr<BeShader> _directionalLightShader;
    std::unique_ptr<BeShader> _pointLightShader;
    
    
public:
    explicit BeLightingPass() = default;
    ~BeLightingPass() override = default;

    [[nodiscard]] auto GetInputResources() const -> std::vector<std::string> override;
    [[nodiscard]] auto GetOutputResources() const -> std::vector<std::string> override;
    auto Initialise() -> void override;
    auto Render() -> void override;
};
