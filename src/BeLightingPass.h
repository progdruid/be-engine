#pragma once
#include <d3d11.h>

#include "BeBuffers.h"
#include "BeRenderPass.h"

class BeShader;
class BeRenderer;

class BeLightingPass final : public BeRenderPass {
public:
    DirectionalLightData DirectionalLightData;
    std::vector<PointLightData> PointLights;

    std::string InputTexture0Name;
    std::string InputTexture1Name;
    std::string InputTexture2Name;
    std::string InputDepthTextureName;
    std::string OutputTextureName;
    
private:
    ComPtr<ID3D11BlendState> _lightingBlendState;

    ComPtr<ID3D11Buffer> _directionalLightBuffer;
    ComPtr<ID3D11Buffer> _pointLightBuffer;
    //ComPtr<ID3D11Buffer> _spotLightBuffer;

    std::unique_ptr<BeShader> _directionalLightShader;
    std::unique_ptr<BeShader> _pointLightShader;
    
    
public:
    explicit BeLightingPass();
    ~BeLightingPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
};
