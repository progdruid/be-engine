#pragma once

#include <memory>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderPass.h"
#include "BeMaterialBinding.h"

class BeTexture;
class BeMaterial;
class BeShader;

class BeBloomPass final : public BeRenderPass {

    expose
    std::weak_ptr<BeTexture> InputHDRTexture;
    std::vector<std::weak_ptr<BeTexture>> BloomMipTextures;
    uint32_t BloomMipCount;
    std::weak_ptr<BeTexture> OutputTexture;
    std::weak_ptr<BeTexture> DirtTexture;

    hide
    std::shared_ptr<BeShader> _brightShader;
    std::shared_ptr<BeMaterial> _brightMaterial;
    BeMaterialBinding _brightBinding;
    SenPipeline _brightPipeline;

    std::shared_ptr<BeShader> _kawaseShader;
    std::vector<std::shared_ptr<BeMaterial>> _downsampleMaterials;
    std::vector<BeMaterialBinding> _downsampleBindings;
    std::vector<std::shared_ptr<BeMaterial>> _upsampleMaterials;
    std::vector<BeMaterialBinding> _upsampleBindings;
    SenPipeline _downsamplePipeline;
    SenPipeline _upsamplePipeline;

    std::shared_ptr<BeShader> _addShader;
    std::shared_ptr<BeMaterial> _addMaterial;
    BeMaterialBinding _addBinding;
    SenPipeline _addPipeline;
    
    expose
    explicit BeBloomPass();
    ~BeBloomPass() override;

    auto Initialise() -> void override;
    auto Render() -> void override;
    auto GetPassName() const -> const std::string override { return "Bloom Pass"; }

    auto GetBrightMaterial() const -> std::weak_ptr<BeMaterial> { return _brightMaterial; }
    
    hide
    auto RenderBrightPass() -> void;
    auto RenderDownsamplePasses() -> void;
    auto RenderUpsamplePasses() -> void;
    auto RenderAddPass() -> void;
};
