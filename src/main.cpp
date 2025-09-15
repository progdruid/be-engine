#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cassert>
#include <vector>

static auto error_callback(int code, const char* desc) -> void {
    (void)std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

template <typename T>
static void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

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

static bool CompileShader(const char* source, const char* entry, const char* target,
                          ID3DBlob** outBlob) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3D10_SHADER_DEBUG | D3D10_SHADER_OPTIMIZATION_LEVEL0;
#endif
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr,
                            entry, target, flags, 0, &code, &errors);
    if (FAILED(hr)) {
        if (errors) {
            std::fprintf(stderr, "HLSL compile error:\n%.*s\n",
                         (int)errors->GetBufferSize(),
                         (const char*)errors->GetBufferPointer());
            errors->Release();
        }
        SafeRelease(code);
        return false;
    }
    if (errors) errors->Release();
    *outBlob = code;
    return true;
}

int main() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    // No client API: Direct3D will render
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "GLFW + D3D11 Triangle", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    HWND hwnd = glfwGetWin32Window(window);
    assert(hwnd != nullptr);

    // Create D3D11 device and context
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL createdFL{};
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags,
        featureLevels, _countof(featureLevels),
        D3D11_SDK_VERSION, &device, &createdFL, &context
    );
    if (FAILED(hr)) {
        std::fprintf(stderr, "D3D11CreateDevice failed: 0x%08X\n", hr);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    // Create factory and swap chain (flip-model)
    IDXGIDevice* dxgiDevice = nullptr;
    hr = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        std::fprintf(stderr, "Query IDXGIDevice failed: 0x%08X\n", hr);
        SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) {
        std::fprintf(stderr, "GetAdapter failed: 0x%08X\n", hr);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    IDXGIFactory2* factory = nullptr;
    hr = adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&factory);
    if (FAILED(hr)) {
        std::fprintf(stderr, "GetParent IDXIFFactory2 failed: 0x%08X\n", hr);
        SafeRelease(adapter); SafeRelease(dxgiDevice);
        SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = fbW;
    scDesc.Height = fbH;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SampleDesc.Count = 1;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    IDXGISwapChain1* swapchain = nullptr;
    hr = factory->CreateSwapChainForHwnd(device, hwnd, &scDesc, nullptr, nullptr, &swapchain);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreateSwapChainForHwnd failed: 0x%08X\n", hr);
        SafeRelease(factory); SafeRelease(adapter); SafeRelease(dxgiDevice);
        SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    // Create RTV
    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) {
        std::fprintf(stderr, "GetBuffer failed: 0x%08X\n", hr);
        SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter); SafeRelease(dxgiDevice);
        SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    ID3D11RenderTargetView* rtv = nullptr;
    hr = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
    SafeRelease(backBuffer);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreateRenderTargetView failed: 0x%08X\n", hr);
        SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter); SafeRelease(dxgiDevice);
        SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    // Compile shaders
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    if (!CompileShader(g_VS_HLSL, "main", "vs_5_0", &vsBlob)) {
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    if (!CompileShader(g_PS_HLSL, "main", "ps_5_0", &psBlob)) {
        SafeRelease(vsBlob);
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreateVertexShader failed: 0x%08X\n", hr);
        SafeRelease(psBlob); SafeRelease(vsBlob);
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreatePixelShader failed: 0x%08X\n", hr);
        SafeRelease(vs); SafeRelease(psBlob); SafeRelease(vsBlob);
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    // Input layout: POSITION (float3), COLOR (float3)
    D3D11_INPUT_ELEMENT_DESC ilDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* inputLayout = nullptr;
    hr = device->CreateInputLayout(ilDesc, _countof(ilDesc),
                                   vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                   &inputLayout);
    SafeRelease(psBlob);
    SafeRelease(vsBlob);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreateInputLayout failed: 0x%08X\n", hr);
        SafeRelease(ps); SafeRelease(vs);
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    // Vertex buffer for a colored triangle (NDC space)
    struct Vertex { float px, py, pz; float r, g, b; };
    Vertex vertices[] = {
        {  0.0f,  0.5f, 0.0f, 1.f, 0.f, 0.f }, // Top - Red
        {  0.5f, -0.5f, 0.0f, 0.f, 1.f, 0.f }, // Right - Green
        { -0.5f, -0.5f, 0.0f, 0.f, 0.f, 1.f }, // Left - Blue
    };
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = UINT(sizeof(vertices));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices;
    ID3D11Buffer* vb = nullptr;
    hr = device->CreateBuffer(&vbDesc, &vbData, &vb);
    if (FAILED(hr)) {
        std::fprintf(stderr, "CreateBuffer(vb) failed: 0x%08X\n", hr);
        SafeRelease(inputLayout); SafeRelease(ps); SafeRelease(vs);
        SafeRelease(rtv); SafeRelease(swapchain); SafeRelease(factory); SafeRelease(adapter);
        SafeRelease(dxgiDevice); SafeRelease(context); SafeRelease(device);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    // Viewport
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f; vp.TopLeftY = 0.0f;
    vp.Width = float(fbW); vp.Height = float(fbH);
    vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int newW = 0, newH = 0;
        glfwGetFramebufferSize(window, &newW, &newH);
        if (newW == 0 || newH == 0) {
            // Minimized; skip rendering this frame
            Sleep(10);
            continue;
        }
        if (newW != fbW || newH != fbH) {
            fbW = newW; fbH = newH;
            SafeRelease(rtv);
            HRESULT rhr = swapchain->ResizeBuffers(2, fbW, fbH, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            if (FAILED(rhr)) {
                std::fprintf(stderr, "ResizeBuffers failed: 0x%08X\n", rhr);
            }
            ID3D11Texture2D* bb = nullptr;
            rhr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
            if (SUCCEEDED(rhr)) {
                rhr = device->CreateRenderTargetView(bb, nullptr, &rtv);
                SafeRelease(bb);
            }
            if (FAILED(rhr)) {
                std::fprintf(stderr, "Recreate RTV after resize failed: 0x%08X\n", rhr);
            }
            vp.Width = float(fbW);
            vp.Height = float(fbH);
        }

        // Bind pipeline state
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->RSSetViewports(1, &vp);
        context->IASetInputLayout(inputLayout);
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs, nullptr, 0);
        context->PSSetShader(ps, nullptr, 0);

        // Clear and draw
        const float clearColor[24] = { 0.1f, 0.6f, 0.6f, 1.0f };
        context->ClearRenderTargetView(rtv, clearColor);
        context->Draw(3, 0);

        // Present with vsync
        swapchain->Present(1, 0);
    }

    // Cleanup
    SafeRelease(vb);
    SafeRelease(inputLayout);
    SafeRelease(ps);
    SafeRelease(vs);
    SafeRelease(rtv);
    SafeRelease(swapchain);
    SafeRelease(factory);
    SafeRelease(adapter);
    SafeRelease(dxgiDevice);
    SafeRelease(context);
    SafeRelease(device);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
