#include "Renderer.h"

#include <d3dcompiler.h>

#include "DXUtils.h"
#include "glm.hpp"
#include "StandardGeometry.h"

Renderer::Renderer(HWND windowHandle, int width, int height) {

    _active = false;

    _windowHandle = windowHandle;
    _width = width;
    _height = height;

    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Device and context
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        deviceFlags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &_device,
        nullptr,
        &_context
    ));

    // DXGI interfaces
    DX::ThrowIfFailed(_device.As(&_dxgiDevice));
    DX::ThrowIfFailed(_dxgiDevice->GetAdapter(&_adapter));
    DX::ThrowIfFailed(_adapter->GetParent(IID_PPV_ARGS(&_factory)));
    
    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = _width;
    scDesc.Height = _height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SampleDesc.Count = 1;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    DX::ThrowIfFailed(_factory->CreateSwapChainForHwnd(_device.Get(), _windowHandle, &scDesc, nullptr, nullptr, &_swapchain));
    DX::ThrowIfFailed(_factory->MakeWindowAssociation(_windowHandle, DXGI_MWA_NO_ALT_ENTER));

    // Create RTV
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(_swapchain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
    DX::ThrowIfFailed(_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_renderTarget));
    backBuffer.Reset();

    // Compile shaders
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescriptors{
        {.SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0,
            .AlignedByteOffset = 0, .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA, .InstanceDataStepRate = 0 },
        {.SemanticName = "COLOR", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0,
            .AlignedByteOffset = 12, .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA, .InstanceDataStepRate = 0 },
    };
    _shader = std::make_shared<Shader>(_device.Get(), L"src/shaders/default", inputElementDescriptors);

    constexpr glm::vec3 vertices[] = {
        StandardGeometry::Square::Vertices[0], {1.0f, 0.0f, 0.0f}, // Top (Red)
        StandardGeometry::Square::Vertices[1],  {0.0f, 1.0f, 0.0f}, // Right (Green)
        StandardGeometry::Square::Vertices[2],  {0.0f, 0.0f, 1.0f}, // Left (Blue)
        StandardGeometry::Square::Vertices[3],  {1.0f, 1.0f, 0.0f}, // Bottom (Yellow)
    };
    D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescriptor.ByteWidth = sizeof(vertices);
    vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;
    DX::ThrowIfFailed(_device->CreateBuffer(&vertexBufferDescriptor, &vertexData, &_vertexBuffer));

    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = sizeof(StandardGeometry::Square::Indices);
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = StandardGeometry::Square::Indices;
    DX::ThrowIfFailed(_device->CreateBuffer(&indexBufferDescriptor, &indexData, &_indexBuffer));
    
    _active = true;
}

auto Renderer::render() -> void {
    auto fullClearColor = glm::vec4(ClearColor, 1.0f);
    _context->ClearRenderTargetView(_renderTarget.Get(), reinterpret_cast<FLOAT*>(&fullClearColor));
    _context->OMSetRenderTargets(1, _renderTarget.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(_width);
    viewport.Height = static_cast<FLOAT>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);
    
    _shader->bind(_context.Get());

    constexpr UINT stride = 6 * sizeof(float);
    constexpr UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
    _context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    _context->DrawIndexed(_countof(StandardGeometry::Square::Indices), 0, 0);
    
    _swapchain->Present(1, 0);
}

auto Renderer::terminateRenderer() -> void {
    _active = false;
    
    _renderTarget.Reset();
    _swapchain.Reset();
    _factory.Reset();
    _adapter.Reset();
    _dxgiDevice.Reset();
    _context.Reset();
    _device.Reset();
}