#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <umbrellas/access-modifiers.hpp>

#include "../RhiSwapchain.h"

using Microsoft::WRL::ComPtr;

class Dx11Swapchain final : public RhiSwapchain {

    hide
    ComPtr<IDXGISwapChain1> _swapchain;

    expose
    explicit Dx11Swapchain(ComPtr<IDXGISwapChain1> swapchain)
        : _swapchain(std::move(swapchain))
    {}

    ~Dx11Swapchain() override = default;

    auto Present(uint32_t syncInterval) -> void override {
        _swapchain->Present(syncInterval, 0);
    }

    auto Resize(uint32_t width, uint32_t height) -> void override {
        _swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    }

    auto GetNative() const -> ComPtr<IDXGISwapChain1> { return _swapchain; }
};
