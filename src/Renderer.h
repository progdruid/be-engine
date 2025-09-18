#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <string>

class Renderer {
public:
    explicit Renderer(HWND windowHandle, int width, int height);
    ~Renderer() = default;

private:
    bool _active = false;

    HWND _windowHandle;
    int _width;
    int _height;
    
    ComPtr<ID3D11Device> _device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIDevice> _dxgiDevice;
    ComPtr<IDXGIAdapter> _adapter;
    ComPtr<IDXGIFactory2> _factory;
    ComPtr<IDXGISwapChain1> _swapchain;
    ComPtr<ID3D11RenderTargetView> _renderTarget;
    ComPtr<ID3D11VertexShader> _vs;
    ComPtr<ID3D11PixelShader> _ps;
    ComPtr<ID3D11InputLayout> _inputLayout;
    ComPtr<ID3D11Buffer> _vertexBuffer;
    ComPtr<ID3D11Buffer> _indexBuffer;

public:
    [[nodiscard]] auto isActive() const -> bool { return _active; }

    auto render() -> void;
    
private:
    auto terminateRenderer() -> void;
};
