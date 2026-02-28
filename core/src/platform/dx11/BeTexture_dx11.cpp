#include "BeTexture.h"
#include "BeRenderer.h"
#include "BeTextureImpl.h"
#include "BeRendererImpl.h"
#include "DxUtils.h"

#include <unordered_map>

static auto ToDxgiFormat(BeTextureFormat format) -> DXGI_FORMAT {
    static const std::unordered_map<BeTextureFormat, DXGI_FORMAT> map = {
        { BeTextureFormat::Unknown,           DXGI_FORMAT_UNKNOWN },
        { BeTextureFormat::R8G8B8A8_UNorm,    DXGI_FORMAT_R8G8B8A8_UNORM },
        { BeTextureFormat::R11G11B10_Float,   DXGI_FORMAT_R11G11B10_FLOAT },
        { BeTextureFormat::R16G16B16A16_Float,DXGI_FORMAT_R16G16B16A16_FLOAT },
        { BeTextureFormat::R32_Typeless,      DXGI_FORMAT_R32_TYPELESS },
        { BeTextureFormat::R24G8_Typeless,    DXGI_FORMAT_R24G8_TYPELESS },
        { BeTextureFormat::R16_Typeless,      DXGI_FORMAT_R16_TYPELESS },
        { BeTextureFormat::R32_Float,         DXGI_FORMAT_R32_FLOAT },
        { BeTextureFormat::R16_UNorm,         DXGI_FORMAT_R16_UNORM },
        { BeTextureFormat::R8_UNorm,          DXGI_FORMAT_R8_UNORM },
    };
    return map.at(format);
}

static auto ToDxBindFlags(BeBindFlags flags) -> uint32_t {
    uint32_t result = 0;
    if (HasAny(flags, BeBindFlags::ShaderResource)) result |= D3D11_BIND_SHADER_RESOURCE;
    if (HasAny(flags, BeBindFlags::RenderTarget))   result |= D3D11_BIND_RENDER_TARGET;
    if (HasAny(flags, BeBindFlags::DepthStencil))   result |= D3D11_BIND_DEPTH_STENCIL;
    if (HasAny(flags, BeBindFlags::VertexBuffer))   result |= D3D11_BIND_VERTEX_BUFFER;
    if (HasAny(flags, BeBindFlags::IndexBuffer))    result |= D3D11_BIND_INDEX_BUFFER;
    if (HasAny(flags, BeBindFlags::ConstantBuffer)) result |= D3D11_BIND_CONSTANT_BUFFER;
    return result;
}

static auto GetDepthSRVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static const std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        {DXGI_FORMAT_R32_TYPELESS,  DXGI_FORMAT_R32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS},
        {DXGI_FORMAT_R16_TYPELESS,  DXGI_FORMAT_R16_UNORM},
    };
    return map.at(textureFormat);
}

static auto GetDSVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static const std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        {DXGI_FORMAT_R32_TYPELESS,  DXGI_FORMAT_D32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT},
        {DXGI_FORMAT_R16_TYPELESS,  DXGI_FORMAT_D16_UNORM},
    };
    return map.at(textureFormat);
}

static auto CreateTexture2DResources(
    ComPtr<ID3D11Device> device,
    BeTextureImpl& impl,
    uint32_t width, uint32_t height, uint32_t mips,
    DXGI_FORMAT format, uint32_t dxBindFlags,
    const uint8_t* defaultData
) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = mips;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = dxBindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
    D3D11_SUBRESOURCE_DATA initData = {};
    if (defaultData) {
        initData.pSysMem = defaultData;
        initData.SysMemPitch = sizeof(uint8_t) * 4 * width;
        initData.SysMemSlicePitch = 0;
        pInitData = &initData;
    }

    DxUtils::Check << device->CreateTexture2D(&textureDesc, pInitData, impl.texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = GetDSVFormat(format);
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        DxUtils::Check << device->CreateDepthStencilView(impl.texture.Get(), &dsvDesc, impl.dsv.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        impl.mipRTVs.resize(mips);
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        for (uint32_t mip = 0; mip < mips; ++mip) {
            rtvDesc.Texture2D.MipSlice = mip;
            DxUtils::Check << device->CreateRenderTargetView(impl.texture.Get(), &rtvDesc, impl.mipRTVs[mip].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? GetDepthSRVFormat(format) : format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = mips;
        DxUtils::Check << device->CreateShaderResourceView(impl.texture.Get(), &srvDesc, impl.srv.GetAddressOf());
    }
}

static auto CreateCubemapResources(
    ComPtr<ID3D11Device> device,
    BeTextureImpl& impl,
    uint32_t width, uint32_t height, uint32_t mips,
    DXGI_FORMAT format, uint32_t dxBindFlags,
    const uint8_t* defaultData
) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = mips;
    textureDesc.ArraySize = 6;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = dxBindFlags;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
    D3D11_SUBRESOURCE_DATA initData = {};
    if (defaultData) {
        initData.pSysMem = defaultData;
        initData.SysMemPitch = sizeof(uint8_t) * 4 * width;
        pInitData = &initData;
    }

    DxUtils::Check << device->CreateTexture2D(&textureDesc, pInitData, impl.texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? GetDepthSRVFormat(format) : format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = mips;
        DxUtils::Check << device->CreateShaderResourceView(impl.texture.Get(), &srvDesc, impl.srv.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (uint32_t face = 0; face < 6; face++) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = GetDSVFormat(format);
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize = 1;
            DxUtils::Check << device->CreateDepthStencilView(impl.texture.Get(), &dsvDesc, impl.cubemapDSVs[face].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        for (uint32_t face = 0; face < 6; face++) {
            impl.cubemapMipRTVs[face].resize(mips);
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;
            for (uint32_t mip = 0; mip < mips; ++mip) {
                rtvDesc.Texture2DArray.MipSlice = mip;
                DxUtils::Check << device->CreateRenderTargetView(impl.texture.Get(), &rtvDesc, impl.cubemapMipRTVs[face][mip].GetAddressOf());
            }
        }
    }
}

BeTexture::~BeTexture() = default;

auto BeTexture::CreatePlatformResources(BeRenderer& renderer, const uint8_t* initialData) -> void {
    _impl = std::make_unique<BeTextureImpl>();
    auto device = renderer.GetPlatformImpl()->device;
    auto dxFormat = ToDxgiFormat(Format);
    auto dxBindFlags = ToDxBindFlags(BindFlags);

    if (!IsCubemap) {
        CreateTexture2DResources(device, *_impl, Width, Height, Mips, dxFormat, dxBindFlags, initialData);
    } else {
        CreateCubemapResources(device, *_impl, Width, Height, Mips, dxFormat, dxBindFlags, initialData);
    }
}
