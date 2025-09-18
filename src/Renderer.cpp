#include "Renderer.h"
#include <cstdio>
#include "DXUtils.h"

static bool CompileShader(const char* source, const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob) {

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3D10_SHADER_DEBUG | D3D10_SHADER_OPTIMIZATION_LEVEL0;
#endif
    ComPtr<ID3DBlob> code;
    ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr,
                            entry, target, flags, 0, code.GetAddressOf(), errors.GetAddressOf());
    if (FAILED(hr)) {
        if (errors) {
            std::fprintf(stderr, "HLSL compile error:\n%.*s\n",
                         (int)errors->GetBufferSize(),
                         (const char*)errors->GetBufferPointer());
        }
        return false;
    }
    outBlob = code;
    return true;
}

// Inline HLSL: position+color passthrough VS, color PS
static const char* g_VS_HLSL = R"(
struct VSInput {
    float3 position : POSITION;
    float3 color    : COLOR0;
};
struct VSOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};
VSOutput main(VSInput input) {
    VSOutput o;
    o.position = float4(input.position, 1.0f);
    o.color = input.color;
    return o;
}
)";

static const char* g_PS_HLSL = R"(
struct PSInput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};
float4 main(PSInput input) : SV_Target {
    return float4(input.color, 1.0f);
}
)";

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
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    DX::ThrowIfFailed(CompileShader(g_VS_HLSL, "main", "vs_5_0", vsBlob));
    DX::ThrowIfFailed(CompileShader(g_PS_HLSL, "main", "ps_5_0", psBlob));
    DX::ThrowIfFailed(_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, _vs.GetAddressOf()));
    DX::ThrowIfFailed(_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, _ps.GetAddressOf()));

    // Input layout: POSITION (float3), COLOR (float3)
    D3D11_INPUT_ELEMENT_DESC ilDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
    DX::ThrowIfFailed(_device->CreateInputLayout(
        ilDesc,
        _countof(ilDesc),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &_inputLayout));

    vsBlob.Reset();
    psBlob.Reset();

    
    constexpr float vertices[] = {
        // Position //-------// Color //-------//
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Top (Red)
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Right (Green)
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, // Left (Blue)
    };
    D3D11_BUFFER_DESC vertexBufferDescriptor = {};
    vertexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDescriptor.ByteWidth = sizeof(vertices);
    vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;
    DX::ThrowIfFailed(_device->CreateBuffer(&vertexBufferDescriptor, &vertexData, &_vertexBuffer));

    constexpr uint32_t indices[] = {
        0, 1, 2
    };
    D3D11_BUFFER_DESC indexBufferDescriptor = {};
    indexBufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDescriptor.ByteWidth = sizeof(indices);
    indexBufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;
    DX::ThrowIfFailed(_device->CreateBuffer(&indexBufferDescriptor, &indexData, &_indexBuffer));
    
    _active = true;
}

auto Renderer::render() -> void {
    const float clearColor[4] = {0.071f, 0.04f, 0.561f, 1.0f};
    _context->ClearRenderTargetView(_renderTarget.Get(), clearColor);
    _context->OMSetRenderTargets(1, _renderTarget.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(_width);
    viewport.Height = static_cast<FLOAT>(_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);
    
    _context->IASetInputLayout(_inputLayout.Get());
    constexpr UINT stride = 6 * sizeof(float);
    constexpr UINT offset = 0;
    _context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
    _context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    _context->VSSetShader(_vs.Get(), nullptr, 0);
    _context->PSSetShader(_ps.Get(), nullptr, 0);
    
    _context->DrawIndexed(3, 0, 0);
    
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