#pragma once
#include <d3d11.h>
#include <memory>
#include <umbrellas/access-modifiers.hpp>
#include <wrl/client.h>

#include "BeShader.h"
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

    std::shared_ptr<BeShader> _boundShader;

    // Cached shader COM pointers from last bound pipeline (for binding decisions)
    ComPtr<ID3D11VertexShader>   _boundVertexShader;
    ComPtr<ID3D11HullShader>     _boundHullShader;
    ComPtr<ID3D11DomainShader>   _boundDomainShader;
    ComPtr<ID3D11PixelShader>    _boundPixelShader;

    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> _vertexResCache;
    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> _tessResCache;
    std::array<uint32_t, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> _pixelResCache;

    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> _vertexCBufferIDCache;
    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT>   _tessCBufferIDCache;
    std::array<uint32_t, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT>  _pixelCBufferIDCache;
    
    std::array<uint32_t, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> _vertexSamplerCache;
    std::array<uint32_t, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT>   _tessSamplerCache;
    std::array<uint32_t, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT>  _pixelSamplerCache;
    
    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide BePipeline() = default;
    expose ~BePipeline() = default;

    
    // interface ///////////////////////////////////////////////////////////////////////////////////////////////////////
    expose
    auto GetRawContext () -> ComPtr<ID3D11DeviceContext> { return _context; }
    
    expose
    auto BindPipeline (SenPipeline pipeline) -> void;
    auto BindMaterialAutomatic (const std::shared_ptr<BeMaterial>& material, const std::shared_ptr<BeShader>& shader = nullptr) -> void;
    auto BindMaterialManual (const std::shared_ptr<BeMaterial>& material, const uint8_t materialSlot) -> void;
    auto Clear() -> void;
    auto ClearCache() -> void;
    
    hide
    auto BindMaterialTextures (const BeMaterial& material) -> void;

    expose
    auto BindTargets (
        const std::vector<std::weak_ptr<BeTexture>>& renderTargets,
        const BeTexture* depthTarget,
        bool clearRTVs = false
    ) const -> void;
    auto ClearTargets () const -> void;
    auto ResetTarget (const std::shared_ptr<BeTexture>& texture) const -> void;

    expose
    auto BindVertexBuffer  (SenBuffer buffer, uint32_t stride) const -> void;
    auto BindIndexBuffer   (SenBuffer buffer) const -> void;
    auto ClearVertexBuffer () const -> void;
    auto ClearIndexBuffer  () const -> void;

    auto Draw(uint32_t vertexCount, uint32_t startVertexLocation) const -> void;
    auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) const -> void;
};
