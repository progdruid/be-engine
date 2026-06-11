#pragma once
#include <memory>
#include <string>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>
#include "BeRenderPass.h"

class BeTexture;
class BeMaterial;

class BeTestComputePass : public BeRenderPass {
    hide
    std::shared_ptr<BeTexture> _input;
    std::shared_ptr<BeTexture> _output;
    std::shared_ptr<BeMaterial> _material;
    SenPipeline _pipeline;

    expose
    BeTestComputePass(std::shared_ptr<BeTexture> input, std::shared_ptr<BeTexture> output);

    auto Initialise () -> void override;
    auto Render () -> void override;
    auto GetPassName () const -> const std::string override { return "TestCompute"; }
};
