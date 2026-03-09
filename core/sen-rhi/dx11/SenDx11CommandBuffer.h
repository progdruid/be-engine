#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

using Microsoft::WRL::ComPtr;

class SenDx11CommandBuffer {
    hide
    ComPtr<ID3D11DeviceContext> _context;

    // Cached shader COM pointers from last bound pipeline (DX11-specific: for stage-aware resource binding)
    ComPtr<ID3D11VertexShader>  _boundVertexShader;
    ComPtr<ID3D11HullShader>    _boundHullShader;
    ComPtr<ID3D11DomainShader>  _boundDomainShader;
    ComPtr<ID3D11PixelShader>   _boundPixelShader;

    expose
    SenDx11CommandBuffer() = default;
    explicit SenDx11CommandBuffer(const ComPtr<ID3D11DeviceContext>& context);

    // render pass
    auto BeginPass (const SenPassDesc& desc) -> void;
    auto EndPass () -> void;

    // pipeline + resources
    auto SetPipeline   (SenPipeline pipeline) -> void;
    auto SetBindGroup  (SenBindGroup group, uint8_t index) -> void;
    auto SetVertexBuffer (SenBuffer buffer, uint32_t stride) -> void;
    auto SetIndexBuffer  (SenBuffer buffer) -> void;
    auto ClearVertexBuffer () -> void;
    auto ClearIndexBuffer  () -> void;

    // draw
    auto Draw        (uint32_t vertexCount, uint32_t firstVertex) -> void;
    auto DrawIndexed (uint32_t indexCount, uint32_t firstIndex, int32_t baseVertex) -> void;
};
