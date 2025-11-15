#pragma once
#include <d3d11.h>

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
    ComPtr<ID3D11Buffer> _objectBuffer;
    
public:
    explicit BeGeometryPass();
    ~BeGeometryPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Geometry Pass"; }
};
