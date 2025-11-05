#pragma once
#include <cstdint>
#include <wrl/client.h>
#include <d3d11.h>
#include <string>

using Microsoft::WRL::ComPtr;

class BeRenderResource {
public:

    struct BeResourceDescriptor {
        DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        uint32_t CustomWidth = 0;
        uint32_t CustomHeight = 0;
    };
    
    //fields////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::string Name;
    BeResourceDescriptor Descriptor;
    
    ComPtr<ID3D11Texture2D> Texture;
    ComPtr<ID3D11RenderTargetView> RTV;
    ComPtr<ID3D11ShaderResourceView> SRV;
    ComPtr<ID3D11DepthStencilView> DSV;

public:
    explicit BeRenderResource(std::string name, const BeResourceDescriptor& descriptor);
    ~BeRenderResource();

    auto CreateGPUResources(const ComPtr<ID3D11Device>& device) -> void;
};
