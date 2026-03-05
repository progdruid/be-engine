#include "SenDx11TextureTable.h"

#include <unordered_map>
#include "Utils.h"
#include <umbrellas/include-libassert.h>
#include <sen-rhi/dx11/SenDx11Convert.h>

// ─── format helpers ───────────────────────────────────────────────────────────

static auto DepthSRVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        { DXGI_FORMAT_R32_TYPELESS,   DXGI_FORMAT_R32_FLOAT              },
        { DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS  },
        { DXGI_FORMAT_R16_TYPELESS,   DXGI_FORMAT_R16_UNORM              },
    };
    return map.at(textureFormat);
}

static auto DepthDSVFormat(DXGI_FORMAT textureFormat) -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> map = {
        { DXGI_FORMAT_R32_TYPELESS,   DXGI_FORMAT_D32_FLOAT        },
        { DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT },
        { DXGI_FORMAT_R16_TYPELESS,   DXGI_FORMAT_D16_UNORM         },
    };
    return map.at(textureFormat);
}

// ─── creation helpers ─────────────────────────────────────────────────────────

static auto Create2D(
    const ComPtr<ID3D11Device>& device,
    const SenTextureDesc& desc,
    SenDx11TextureEntry& entry
) -> void {
    const DXGI_FORMAT dxFormat    = Sen::Dx11::ToFormat(desc.format);
    const uint32_t    dxBindFlags = Sen::Dx11::ToBindFlags(desc.usage);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = desc.width;
    td.Height           = desc.height;
    td.MipLevels        = desc.mips;
    td.ArraySize        = 1;
    td.Format           = dxFormat;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = dxBindFlags;

    D3D11_SUBRESOURCE_DATA  initData = {};
    D3D11_SUBRESOURCE_DATA* pInit    = nullptr;
    if (desc.data) {
        initData.pSysMem          = desc.data;
        initData.SysMemPitch      = sizeof(uint8_t) * 4 * desc.width;
        initData.SysMemSlicePitch = 0;
        pInit = &initData;
    }

    Utils::Check << device->CreateTexture2D(&td, pInit, entry.Texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format             = DepthDSVFormat(dxFormat);
        dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        Utils::Check << device->CreateDepthStencilView(entry.Texture.Get(), &dsvDesc, entry.DSV.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        entry.MipRTVs.resize(desc.mips);
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format        = dxFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        for (uint32_t mip = 0; mip < desc.mips; ++mip) {
            rtvDesc.Texture2D.MipSlice = mip;
            Utils::Check << device->CreateRenderTargetView(entry.Texture.Get(), &rtvDesc, entry.MipRTVs[mip].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? DepthSRVFormat(dxFormat) : dxFormat;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels       = desc.mips;
        Utils::Check << device->CreateShaderResourceView(entry.Texture.Get(), &srvDesc, entry.SRV.GetAddressOf());
    }
}

static auto CreateCubemap(
    const ComPtr<ID3D11Device>& device,
    const SenTextureDesc& desc,
    SenDx11TextureEntry& entry
) -> void {
    const DXGI_FORMAT dxFormat    = Sen::Dx11::ToFormat(desc.format);
    const uint32_t    dxBindFlags = Sen::Dx11::ToBindFlags(desc.usage);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = desc.width;
    td.Height           = desc.height;
    td.MipLevels        = desc.mips;
    td.ArraySize        = 6;
    td.Format           = dxFormat;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = dxBindFlags;
    td.MiscFlags        = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA  initData = {};
    D3D11_SUBRESOURCE_DATA* pInit    = nullptr;
    if (desc.data) {
        initData.pSysMem          = desc.data;
        initData.SysMemPitch      = sizeof(uint8_t) * 4 * desc.width;
        initData.SysMemSlicePitch = 0;
        pInit = &initData;
    }

    Utils::Check << device->CreateTexture2D(&td, pInit, entry.Texture.GetAddressOf());

    if (dxBindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                      = (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) ? DepthSRVFormat(dxFormat) : dxFormat;
        srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels       = desc.mips;
        Utils::Check << device->CreateShaderResourceView(entry.Texture.Get(), &srvDesc, entry.SRV.GetAddressOf());
    }

    if (dxBindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (uint32_t face = 0; face < 6; ++face) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format                         = DepthDSVFormat(dxFormat);
            dsvDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice        = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize       = 1;
            Utils::Check << device->CreateDepthStencilView(entry.Texture.Get(), &dsvDesc, entry.CubemapDSVs[face].GetAddressOf());
        }
    }

    if (dxBindFlags & D3D11_BIND_RENDER_TARGET) {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format                         = dxFormat;
        rtvDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.ArraySize       = 1;
        for (uint32_t face = 0; face < 6; ++face) {
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            entry.CubemapMipRTVs[face].resize(desc.mips);
            for (uint32_t mip = 0; mip < desc.mips; ++mip) {
                rtvDesc.Texture2DArray.MipSlice = mip;
                Utils::Check << device->CreateRenderTargetView(entry.Texture.Get(), &rtvDesc, entry.CubemapMipRTVs[face][mip].GetAddressOf());
            }
        }
    }
}

// ─── table ────────────────────────────────────────────────────────────────────

SenDx11TextureTable* SenDx11TextureTable::_instance = nullptr;

auto SenDx11TextureTable::Init() -> void {
    _instance = new SenDx11TextureTable();
}

auto SenDx11TextureTable::Shutdown() -> void {
    delete _instance;
    _instance = nullptr;
}

auto SenDx11TextureTable::Get() -> SenDx11TextureTable& {
    be_assert(_instance, "SenDx11TextureTable: Instance was already destroyed. Shutdown called.");
    return *_instance;
}

auto SenDx11TextureTable::Create(const ComPtr<ID3D11Device>& device, const SenTextureDesc& desc) -> SenTexture {
    const SenTexture handle { _nextId++ };
    auto& entry = _entries[handle.id];

    if (desc.cubemap)
        CreateCubemap(device, desc, entry);
    else
        Create2D(device, desc, entry);

    return handle;
}

auto SenDx11TextureTable::Destroy(SenTexture handle) -> void {
    _entries.erase(handle.id);
}

auto SenDx11TextureTable::Lookup(SenTexture handle) -> SenDx11TextureEntry& {
    return _entries.at(handle.id);
}
