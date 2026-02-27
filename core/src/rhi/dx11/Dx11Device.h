#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiDevice.h"
#include "../RhiFormatConverter.h"

using Microsoft::WRL::ComPtr;

class Dx11Device final : public RhiDevice {

    hide
    ComPtr<ID3D11Device> _device;

    expose
    explicit Dx11Device(ComPtr<ID3D11Device> device)
        : _device(std::move(device))
    {}

    ~Dx11Device() override = default;

    auto CreateBuffer(const RhiBufferDesc& desc, const void* initialData) -> std::shared_ptr<RhiBuffer> override;
    auto CreateSampler(const RhiSamplerDesc& desc) -> std::shared_ptr<RhiSampler> override;

    auto GetBackendName() const -> const char* override { return "DirectX 11"; }

    auto GetNative() const -> ComPtr<ID3D11Device> { return _device; }
    auto GetNativePtr() const -> ID3D11Device* { return _device.Get(); }
};
