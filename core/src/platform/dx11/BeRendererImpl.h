#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeRendererImpl {
    HWND hwnd = nullptr;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> backbufferTarget;

    ComPtr<ID3D11Buffer> uniformBuffer;
    ComPtr<ID3D11DepthStencilState> defaultDepthStencilState;
    ComPtr<ID3D11RasterizerState> rasterizerCullBack;
    ComPtr<ID3D11RasterizerState> rasterizerCullNone;
};
