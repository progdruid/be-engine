#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../IGalSampler.h"

using Microsoft::WRL::ComPtr;

class Dx11Sampler final : public IGalSampler {

    hide
    ComPtr<ID3D11SamplerState> _sampler;

    expose
    explicit Dx11Sampler(ComPtr<ID3D11SamplerState> sampler)
        : _sampler(std::move(sampler))
    {}

    ~Dx11Sampler() override = default;

    auto GetNative() const -> ComPtr<ID3D11SamplerState> { return _sampler; }
    auto GetNativePtr() const -> ID3D11SamplerState* { return _sampler.Get(); }
};
