#pragma once
#include <array>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>

#include "sen-rhi/SenTypes.h"

class BeShader;

class BePipelineBuilder {

    expose struct CachedPipelineKey {
        uint32_t ShaderID;
        SenTopology Topology;
        SenRasterizerState RasterizerState;
        SenBlendState BlendState;
        SenDepthStencilState DepthStencilState;
        std::array<SenFormat, 8> ColorFormats;
        SenFormat DepthFormat;
        
        CachedPipelineKey() { memset(this, 0, sizeof(CachedPipelineKey)); }
        auto operator==(const CachedPipelineKey& k) const -> bool {
            return memcmp(this, &k, sizeof(CachedPipelineKey)) == 0;
        }
    };
    
    hide struct CachedPipelineHash {
        auto operator()(const CachedPipelineKey& k) const -> size_t;
    };

    hide static std::unordered_map<CachedPipelineKey, SenPipeline, CachedPipelineHash> _cachedPipelines;
    hide static std::unordered_map<uint32_t, SenPipeline> _cachedComputePipelines;
    expose static auto Start(const BeShader& shader) -> BePipelineBuilder;
    expose static auto BuildCompute(const BeShader& shader) -> SenPipeline;
    
    hide const SenPipelineDesc* _baseDesc;
    hide CachedPipelineKey _key;
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
