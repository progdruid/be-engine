#pragma once

#include <memory>
#include <vector>

#include "BeRenderPass.h"
#include "umbrellas/access-modifiers.hpp"

class BeAssetRegistry;
class BeMaterial;
class BeShader;

class BeBloomPass final : public BeRenderPass {
    
    expose
    std::string InputHDRTextureName;
    std::vector<std::string> BloomMipTextureNames;
    std::shared_ptr<BeAssetRegistry> AssetRegistry;
    
    hide
    std::shared_ptr<BeShader> _brightShader;
    std::shared_ptr<BeMaterial> _brightMaterial;
    
    std::shared_ptr<BeShader> _downsampleShader;
    std::vector<std::shared_ptr<BeMaterial>> _downsampleMaterials;
    
    std::shared_ptr<BeShader> _upsampleShader;

    expose
    explicit BeBloomPass();
    ~BeBloomPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Bloom Pass"; }

    hide
    auto RenderBrightPass() const -> void;
    auto RenderDownsamplePasses() -> void;
};
