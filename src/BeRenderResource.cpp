#include "BeRenderResource.h"

#include <assert.h>
#include <unordered_map>

#include "Utils.h"

BeRenderResource::BeRenderResource(std::string name, const BeResourceDescriptor& descriptor)
    : Name(std::move(name))
    , Descriptor(descriptor) {
}

BeRenderResource::~BeRenderResource() = default;

auto BeRenderResource::CreateGPUResources(const ComPtr<ID3D11Device>& device) -> void {
    if (!Descriptor.IsCubemap) {
        CreateTexture2DResources(device);
    } else {
        CreateCubemapResources(device);
    }
}

auto BeRenderResource::GetCubemapDSV(const uint32_t faceIndex) -> ComPtr<ID3D11DepthStencilView> {
    assert(Descriptor.IsCubemap, "Resource is not a cubemap");
    assert(faceIndex < 6, "Face index out of bounds");
    return CubemapDSVs[faceIndex];
}

auto BeRenderResource::GetCubemapRTV(const uint32_t faceIndex) -> ComPtr<ID3D11RenderTargetView> {
    assert(Descriptor.IsCubemap, "Resource is not a cubemap");
    assert(faceIndex < 6, "Face index out of bounds");
    return CubemapRTVs[faceIndex];
}

auto BeRenderResource::CreateTexture2DResources(const ComPtr<ID3D11Device>& device) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = Descriptor.CustomWidth;
    textureDesc.Height = Descriptor.CustomHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = Descriptor.Format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = Descriptor.BindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create texture
    Utils::Check << device->CreateTexture2D(&textureDesc, nullptr, Texture.GetAddressOf());
    
    // Create a depth stencil view and its SRV if needed
    if (Descriptor.BindFlags & D3D11_BIND_DEPTH_STENCIL) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = GetDSVFormat(Descriptor.Format);
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        Utils::Check << device->CreateDepthStencilView(Texture.Get(), &dsvDesc, DSV.GetAddressOf());
    }
    
    // Create a render target view if needed
    if (Descriptor.BindFlags & D3D11_BIND_RENDER_TARGET) {
        Utils::Check << device->CreateRenderTargetView(Texture.Get(), nullptr, RTV.GetAddressOf());
    }

    // Create a shader resource view if needed
    if (Descriptor.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            Descriptor.BindFlags & D3D11_BIND_DEPTH_STENCIL
            ? GetDepthSRVFormat(Descriptor.Format)
            : Descriptor.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        Utils::Check << device->CreateShaderResourceView(Texture.Get(), &srvDesc, SRV.GetAddressOf());
    }
}

auto BeRenderResource::CreateCubemapResources(const ComPtr<ID3D11Device>& device) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = Descriptor.CustomWidth;
    textureDesc.Height = Descriptor.CustomHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 6;
    textureDesc.Format = Descriptor.Format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = Descriptor.BindFlags;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    Utils::Check << device->CreateTexture2D(&textureDesc, nullptr, Texture.GetAddressOf());

    if (Descriptor.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            Descriptor.BindFlags & D3D11_BIND_DEPTH_STENCIL
            ? GetDepthSRVFormat(Descriptor.Format)
            : Descriptor.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = 1;
        Utils::Check << device->CreateShaderResourceView(Texture.Get(), &srvDesc, SRV.GetAddressOf());
    }

    // Create individual face DSVs (for depth cubemaps)
    if (Descriptor.BindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (uint32_t face = 0; face < 6; face++) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = GetDSVFormat(Descriptor.Format);
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize = 1;

            Utils::Check << device->CreateDepthStencilView(Texture.Get(), &dsvDesc, CubemapDSVs[face].GetAddressOf());
        }
    }

    // Create individual face RTVs (for color cubemaps, env maps)
    if (Descriptor.BindFlags & D3D11_BIND_RENDER_TARGET) {
        for (uint32_t face = 0; face < 6; face++) {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = Descriptor.Format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;

            Utils::Check << device->CreateRenderTargetView(Texture.Get(), &rtvDesc, CubemapRTVs[face].GetAddressOf());
        }
    }

}

auto BeRenderResource::GetDepthSRVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> textureToSRV = {
        {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS},
        {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM},
    };
    return textureToSRV.at(textureFormat);
}

auto BeRenderResource::GetDSVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> textureToDSV = {
        {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT},
        {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM},
    };
    return textureToDSV.at(textureFormat);
}
