#pragma once

#include <memory>

#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;
class BeShader;

class BeGeometryPass final : public BeRenderPass {
    expose
    std::weak_ptr<BeTexture> OutputTexture0;
    std::weak_ptr<BeTexture> OutputTexture1;
    std::weak_ptr<BeTexture> OutputTexture2;
    std::weak_ptr<BeTexture> OutputDepthTexture;

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
