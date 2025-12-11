#pragma once
#include <d3d11.h>
#include <span>

#include "BeBuffers.h"
#include "BeRenderPass.h"

class BeShader;
class BeRenderer;

class BeLightingPass final : public BeRenderPass {
    expose
    std::weak_ptr<BeDirectionalLight> DirectionalLight;
    std::span<const BePointLight> PointLights;

    std::string InputTexture0Name;
    std::string InputTexture1Name;
    std::string InputTexture2Name;
    std::string InputDepthTextureName;
    std::string OutputTextureName;
    
    hide
    ComPtr<ID3D11BlendState> _lightingBlendState;

    std::shared_ptr<BeShader> _directionalLightShader;
    std::shared_ptr<BeMaterial> _directionalLightMaterial;
    std::shared_ptr<BeShader> _pointLightShader;
    std::shared_ptr<BeMaterial> _pointLightMaterial;
    
    expose
    explicit BeLightingPass();
    ~BeLightingPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Lighting Pass"; }
};
