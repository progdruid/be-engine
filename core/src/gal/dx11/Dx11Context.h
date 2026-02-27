#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../IGalContext.h"
#include "../GalFormatConverter.h"
#include "Dx11Buffer.h"

using Microsoft::WRL::ComPtr;

class Dx11Context final : public IGalContext {

    hide
    ComPtr<ID3D11DeviceContext> _context;

    expose
    explicit Dx11Context(ComPtr<ID3D11DeviceContext> context)
        : _context(std::move(context))
    {}

    ~Dx11Context() override = default;

    auto SetViewport(const GalViewport& viewport) -> void override {
        D3D11_VIEWPORT vp;
        vp.TopLeftX = viewport.X;
        vp.TopLeftY = viewport.Y;
        vp.Width = viewport.Width;
        vp.Height = viewport.Height;
        vp.MinDepth = viewport.MinDepth;
        vp.MaxDepth = viewport.MaxDepth;
        _context->RSSetViewports(1, &vp);
    }

    auto SetTopology(GalTopology topology) -> void override {
        _context->IASetPrimitiveTopology(GalFormatConverter::ToD3DTopology(topology));
    }

    auto MapBuffer(const std::shared_ptr<IGalBuffer>& buffer, void** outData) -> void override {
        auto dx11Buffer = std::static_pointer_cast<Dx11Buffer>(buffer);
        D3D11_MAPPED_SUBRESOURCE mapped;
        _context->Map(dx11Buffer->GetNativePtr(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        *outData = mapped.pData;
    }

    auto UnmapBuffer(const std::shared_ptr<IGalBuffer>& buffer) -> void override {
        auto dx11Buffer = std::static_pointer_cast<Dx11Buffer>(buffer);
        _context->Unmap(dx11Buffer->GetNativePtr(), 0);
    }

    auto Draw(uint32_t vertexCount, uint32_t startVertex) -> void override {
        _context->Draw(vertexCount, startVertex);
    }

    auto DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) -> void override {
        _context->DrawIndexed(indexCount, startIndex, baseVertex);
    }

    auto GetNative() const -> ComPtr<ID3D11DeviceContext> { return _context; }
    auto GetNativePtr() const -> ID3D11DeviceContext* { return _context.Get(); }
};
