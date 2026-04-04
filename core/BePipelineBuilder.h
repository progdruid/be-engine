#pragma once
#include <array>
#include <memory>
#include <unordered_map>
#include <umbrellas/access-modifiers.hpp>

#include "sen-rhi/SenTypes.h"

class BeShader;

class BePipelineBuilder {

    hide struct CachedPipelineKey {
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
    expose static auto Start(std::weak_ptr<BeShader> shader) -> BePipelineBuilder;
    
    hide SenPipelineDesc _desc;
    hide CachedPipelineKey _key;
    hide explicit BePipelineBuilder(SenPipelineDesc desc, uint32_t shaderID);
    expose ~BePipelineBuilder() = default;
    
    expose
    auto SetTopology(SenTopology topology) -> BePipelineBuilder& ;
    auto SetRasterizer (const SenRasterizerState& rasterizer) -> BePipelineBuilder& ;
    auto SetBlend (SenBlendState blend) -> BePipelineBuilder& ;
    auto SetDepthStencil (const SenDepthStencilState& depthStencil) -> BePipelineBuilder& ;
    auto SetTargetFormats (const std::vector<SenFormat>& colorFormats, SenFormat depthFormat = SenFormat::Unknown) -> BePipelineBuilder& ;
    auto Build () const -> SenPipeline;
    
};
