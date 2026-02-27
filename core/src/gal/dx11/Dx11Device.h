#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../IGalDevice.h"
#include "../GalFormatConverter.h"

using Microsoft::WRL::ComPtr;

class Dx11Device final : public IGalDevice {

    hide
    ComPtr<ID3D11Device> _device;

    expose
    explicit Dx11Device(ComPtr<ID3D11Device> device)
        : _device(std::move(device))
    {}

    ~Dx11Device() override = default;

    auto CreateBuffer(const GalBufferDesc& desc, const void* initialData) -> std::shared_ptr<IGalBuffer> override;
    auto CreateSampler(const GalSamplerDesc& desc) -> std::shared_ptr<IGalSampler> override;

    auto GetBackendName() const -> const char* override { return "DirectX 11"; }

    auto GetNative() const -> ComPtr<ID3D11Device> { return _device; }
    auto GetNativePtr() const -> ID3D11Device* { return _device.Get(); }
};
