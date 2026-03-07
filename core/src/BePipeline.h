#pragma once
#include <d3d11.h>
#include <memory>
#include <umbrellas/access-modifiers.hpp>
#include <wrl/client.h>

#include <sen-rhi/SenTypes.h>

class BeTexture;
class BeMaterial;
using Microsoft::WRL::ComPtr;

class BePipeline {

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    static auto Create (const ComPtr<ID3D11DeviceContext>& context) -> std::shared_ptr<BePipeline>;


    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide
    ComPtr<ID3D11DeviceContext> _context;

    // Cached shader COM pointers from last bound pipeline (for stage-aware binding decisions)
    ComPtr<ID3D11VertexShader>   _boundVertexShader;
    ComPtr<ID3D11HullShader>     _boundHullShader;
    ComPtr<ID3D11DomainShader>   _boundDomainShader;
    ComPtr<ID3D11PixelShader>    _boundPixelShader;
    
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide BePipeline() = default;
    expose ~BePipeline() = default;

    
    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto GetRawContext () -> ComPtr<ID3D11DeviceContext> { return _context; }

    expose
    auto BindPipeline  (SenPipeline pipeline) -> void;
    auto SetBindGroup (SenBindGroup group, uint8_t slot) -> void;

    expose
    auto BindVertexBuffer  (SenBuffer buffer, uint32_t stride) const -> void;
    auto BindIndexBuffer   (SenBuffer buffer) const -> void;
    auto ClearVertexBuffer () const -> void;
    auto ClearIndexBuffer  () const -> void;

    auto Draw        (uint32_t vertexCount, uint32_t startVertexLocation) const -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void;
};
