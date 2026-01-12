#pragma once
#include <d3d11.h>
#include <memory>
#include <span>

#include "BeRenderPass.h"

class BeBRPSubmissionBuffer;
class BeTexture;
struct BePointLight;
struct BeDirectionalLight;
class BeMaterial;
class BeShader;
class BeRenderer;

class BeLightingPass final : public BeRenderPass {
    expose
    std::weak_ptr<BeBRPSubmissionBuffer> SubmissionBuffer;

    std::weak_ptr<BeTexture> InputTexture0;
    std::weak_ptr<BeTexture> InputTexture1;
    std::weak_ptr<BeTexture> InputTexture2;
    std::weak_ptr<BeTexture> InputTexture3;
    std::weak_ptr<BeTexture> InputDepthTexture;
    std::weak_ptr<BeTexture> OutputTexture;
    
    hide
    ComPtr<ID3D11BlendState> _lightingBlendState;

    std::shared_ptr<BeShader> _directionalLightShader;
    std::shared_ptr<BeMaterial> _directionalLightMaterial;
    std::shared_ptr<BeShader> _pointLightShader;
    std::shared_ptr<BeMaterial> _pointLightMaterial;
    std::shared_ptr<BeShader> _emissiveAddShader;
    std::shared_ptr<BeMaterial> _emissiveMaterial;
    
    expose
    explicit BeLightingPass();
    ~BeLightingPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Lighting Pass"; }
};
