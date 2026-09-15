#pragma once
#include <initializer_list>
#include <vector>
#include <umbrellas/common.hpp>

#include "BeBackend.h"
#include "sen-rhi/SenTypes.h"

class BeShader;

class BePipelineBuilder {

    expose static auto Start(const BeShader& shader) -> BePipelineBuilder;
    expose static auto BuildCompute(const BeShader& shader) -> SenPipeline;

    hide const SenPipelineDesc* _baseDesc;
    hide BeBackend::PipelineKey _key;
    hide explicit BePipelineBuilder(const SenPipelineDesc& desc, uint32_t shaderID);
    expose ~BePipelineBuilder();

    expose
    auto SetTopology(SenTopology topology) -> BePipelineBuilder&;
    auto SetCullMode(SenCullMode mode) -> BePipelineBuilder&;
    auto SetFillMode(SenFillMode mode) -> BePipelineBuilder&;
    auto SetRasterizer (const SenRasterizerState& rasterizer) -> BePipelineBuilder&;
    auto SetBlend (SenBlendState blend) -> BePipelineBuilder&;
    auto SetDepthStencil (const SenDepthStencilState& depthStencil) -> BePipelineBuilder&;
    auto SetColorFormats (std::initializer_list<SenFormat> colorFormats) -> BePipelineBuilder&;
    auto SetColorFormats (std::vector<SenFormat> colorFormats) -> BePipelineBuilder&;
    auto SetDepthFormat (SenFormat depthFormat = SenFormat::Unknown) -> BePipelineBuilder&;
    auto Build () const -> SenPipeline;

};
