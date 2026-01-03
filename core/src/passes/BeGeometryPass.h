#pragma once

#include <memory>

#include "BeRenderPass.h"

class BeMaterial;
class BeShader;

class BeGeometryPass final : public BeRenderPass {
    expose
    std::string OutputTexture0Name;
    std::string OutputTexture1Name;
    std::string OutputTexture2Name;
    std::string OutputDepthTextureName;

    hide 
    std::shared_ptr<BeMaterial> _objectMaterial;
    
    expose
    explicit BeGeometryPass();
    ~BeGeometryPass() override;

    expose
    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Geometry Pass"; }
};
