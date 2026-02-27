#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiBuffer.h"

using Microsoft::WRL::ComPtr;

class Dx11Buffer final : public RhiBuffer {

    hide
    ComPtr<ID3D11Buffer> _buffer;
    uint32_t _byteWidth;
    RhiBufferUsage _usage;
    RhiBindFlags _bindFlags;

    expose
    explicit Dx11Buffer(ComPtr<ID3D11Buffer> buffer, uint32_t byteWidth, RhiBufferUsage usage, RhiBindFlags bindFlags)
        : _buffer(std::move(buffer))
        , _byteWidth(byteWidth)
        , _usage(usage)
        , _bindFlags(bindFlags)
    {}

    ~Dx11Buffer() override = default;

    auto GetByteWidth() const -> uint32_t override { return _byteWidth; }
    auto GetUsage() const -> RhiBufferUsage override { return _usage; }
    auto GetBindFlags() const -> RhiBindFlags override { return _bindFlags; }

    auto GetNative() const -> ComPtr<ID3D11Buffer> { return _buffer; }
    auto GetNativePtr() const -> ID3D11Buffer* { return _buffer.Get(); }
};
