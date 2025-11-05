#pragma once
#include <d3d11.h>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include "BeModel.h"

#include "BeRenderPass.h"
#include "BeTexture.h"

class BeShader;

class BeGeometryPass final : public BeRenderPass {
public:
    std::string OutputTexture0Name;
    std::string OutputTexture1Name;
    std::string OutputTexture2Name;
    std::string OutputDepthTextureName;
    
private:
    ComPtr<ID3D11Buffer> _materialBuffer;
    ComPtr<ID3D11DepthStencilState> _depthStencilState;
    
    BeTexture _whiteFallbackTexture {glm::vec4(1.0f)};
    
public:
    explicit BeGeometryPass();
    ~BeGeometryPass() override;
    
    auto Initialise() -> void override;
    auto Render() -> void override;
};
