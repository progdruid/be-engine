#include "BeTexture.h"

#include <cassert>
#include <unordered_map>
#include <umbrellas/include-glm.h>
#include <stb_image/stb_image.h>

#include "BeAssetRegistry.h"
#include "Utils.h"


BeTexture::Builder::Builder(std::string name) { _descriptor.Name = std::move(name); }

BeTexture::Builder::~Builder() {
    if (_descriptor.Data) {
        free(_descriptor.Data);
        _descriptor.Data = nullptr;
    }
}

auto BeTexture::Builder::SetBindFlags (uint32_t bindFlags)       -> Builder&& { _descriptor.BindFlags = bindFlags; return std::move(*this); }
auto BeTexture::Builder::SetFormat    (DXGI_FORMAT format)       -> Builder&& { _descriptor.Format = format; return std::move(*this); }
auto BeTexture::Builder::SetMips      (uint32_t mips)            -> Builder&& { _descriptor.Mips = mips; return std::move(*this); }
auto BeTexture::Builder::SetSize      (uint32_t w, uint32_t h)   -> Builder&& { _descriptor.Width = w; _descriptor.Height = h; return std::move(*this); }
auto BeTexture::Builder::SetCubemap   (bool cubemap)             -> Builder&& { _descriptor.IsCubemap = cubemap; return std::move(*this); }

auto BeTexture::Builder::FillWithColor(const glm::vec4& color) -> Builder&& {
    const size_t size = _descriptor.Width * _descriptor.Height;
    const auto data = static_cast<uint8_t*>(malloc(size * 4 * sizeof(uint8_t)));

    for (size_t i = 0; i < size; ++i) {
        data[4 * i + 0] = static_cast<uint8_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
        data[4 * i + 1] = static_cast<uint8_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
        data[4 * i + 2] = static_cast<uint8_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
        data[4 * i + 3] = static_cast<uint8_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
    }

    _descriptor.Data = data;
    return std::move(*this);
}

auto BeTexture::Builder::FillFromMemory(const uint8_t* src) -> Builder&& {
    const size_t byteSize = _descriptor.Width * _descriptor.Height * 4 * sizeof(uint8_t);
    const auto data = static_cast<uint8_t*>(malloc(byteSize));

    memcpy(data, src, byteSize);
    FlipVertically(_descriptor.Width, _descriptor.Height, data);

    _descriptor.Data = data;
    return std::move(*this);
}

auto BeTexture::Builder::LoadFromFile(const std::filesystem::path& file) -> Builder&& {
    int w = 0, h = 0, channelsInFile = 0;
    uint8_t* decoded = stbi_load(file.string().c_str(), &w, &h, &channelsInFile, 4);
    if (!decoded) throw std::runtime_error("Failed to load texture from file: " + file.string());

    const size_t imageSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    const auto data = static_cast<uint8_t*>(malloc(imageSize));
    if (!data) {
        stbi_image_free(decoded);
        throw std::runtime_error("Failed to allocate texture");
    }
    memcpy(data, decoded, imageSize);
    stbi_image_free(decoded);

    FlipVertically(w, h, data);
    _descriptor.Data = data;
    _descriptor.Width = w;
    _descriptor.Height = h;
    return std::move(*this);
}

auto BeTexture::Builder::FlipVertically(const uint32_t w, const uint32_t h, uint8_t* data) -> void {
    const uint32_t rowSize = w * 4; 
    const auto tempRow = new uint8_t[rowSize];

    for (uint32_t y = 0; y < h / 2; ++y) {
        uint8_t* topRow = data + y * rowSize;
        uint8_t* bottomRow = data + (h - 1 - y) * rowSize;

        // swap
        memcpy(tempRow, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow, rowSize);
    }

    delete[] tempRow;
}

auto BeTexture::Builder::AddToRegistry(std::weak_ptr<BeAssetRegistry> registry) -> Builder&& { _assetRegistry = registry; return std::move(*this); }


auto BeTexture::Builder::Build(ComPtr<ID3D11Device> device) -> std::shared_ptr<BeTexture> {
    std::shared_ptr<BeTexture> resource(new BeTexture(device, _descriptor));
    if (!_assetRegistry.expired())
        _assetRegistry.lock()->AddTexture(_descriptor.Name, resource);
    return resource;
}

auto BeTexture::Builder::BuildNoReturn(ComPtr<ID3D11Device> device) -> void {
    const std::shared_ptr<BeTexture> resource(new BeTexture(device, _descriptor));
    if (!_assetRegistry.expired())
        _assetRegistry.lock()->AddTexture(_descriptor.Name, resource);
}


BeTexture::BeTexture(ComPtr<ID3D11Device> device, const BeTextureDescriptor& descriptor)
: Name(descriptor.Name)
, Width(descriptor.Width)
, Height(descriptor.Height)
, IsCubemap(descriptor.IsCubemap)
, Mips(descriptor.Mips)
, BindFlags(descriptor.BindFlags)
, Format(descriptor.Format)
{
    CreateMipViewports();

    if (!descriptor.IsCubemap) {
        CreateTexture2DResources(device, descriptor.Data);
    } else {
        CreateCubemapResources(device, descriptor.Data);
    }
}

BeTexture::~BeTexture() = default;




auto BeTexture::GetMipViewport(const uint32_t mip) const -> const D3D11_VIEWPORT& { return _mipViewports[mip]; }
auto BeTexture::GetSRV() const -> ComPtr<ID3D11ShaderResourceView> { return _srv; }
auto BeTexture::GetDSV() const -> ComPtr<ID3D11DepthStencilView> { return _dsv; }
auto BeTexture::GetRTV(const uint32_t mip) const -> ComPtr<ID3D11RenderTargetView> {
    assert(mip < _mipRTVs.size(), "Mip out of bounds of mip rtv array");
    return _mipRTVs[mip];
}

auto BeTexture::GetCubemapDSV(const uint32_t faceIndex) -> ComPtr<ID3D11DepthStencilView> {
    assert(IsCubemap, "Resource is not a cubemap");
    assert(faceIndex < 6, "Face index out of bounds");
    return _cubemapDSVs[faceIndex];
}

auto BeTexture::GetCubemapRTV(const uint32_t faceIndex, const uint32_t mip) -> ComPtr<ID3D11RenderTargetView> {
    assert(IsCubemap, "Resource is not a cubemap");
    assert(faceIndex < 6, "Face index out of bounds");
    assert(mip < _cubemapMipRTVs[faceIndex].size(), "Mip out of bounds of cubemap's mip rtv array");
    return _cubemapMipRTVs[faceIndex][mip];
}



auto BeTexture::CreateTexture2DResources(ComPtr<ID3D11Device> device, const uint8_t* defaultData) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = Width;
    textureDesc.Height = Height;
    textureDesc.MipLevels = Mips;
    textureDesc.ArraySize = 1;
    textureDesc.Format = Format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = BindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
    if (defaultData) {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = defaultData;
        initData.SysMemPitch = sizeof(uint8_t) * 4 * Width;
        initData.SysMemSlicePitch = 0;
        pInitData = &initData;
    }
    
    Utils::Check << device->CreateTexture2D(&textureDesc, pInitData, _texture.GetAddressOf());
    
    if (BindFlags & D3D11_BIND_DEPTH_STENCIL) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = GetDSVFormat(Format);
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        Utils::Check << device->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf());
    }
    
    if (BindFlags & D3D11_BIND_RENDER_TARGET) {
        _mipRTVs.resize(Mips);
        
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Format;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        for (int mip = 0; mip < Mips; ++mip) {
            rtvDesc.Texture2D.MipSlice = mip;
            Utils::Check << device->CreateRenderTargetView(_texture.Get(), &rtvDesc, _mipRTVs[mip].GetAddressOf());
        }
    }

    if (BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            BindFlags & D3D11_BIND_DEPTH_STENCIL
            ? GetDepthSRVFormat(Format)
            : Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = Mips;
        Utils::Check << device->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf());
    }
}

auto BeTexture::CreateCubemapResources(ComPtr<ID3D11Device> device, const uint8_t* defaultData) -> void {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = Width;
    textureDesc.Height = Height;
    textureDesc.MipLevels = Mips;
    textureDesc.ArraySize = 6;
    textureDesc.Format = Format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = BindFlags;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
    if (defaultData) {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = defaultData;
        initData.SysMemPitch = sizeof(uint8_t) * 4 * Width;
        initData.SysMemSlicePitch = 0;
        pInitData = &initData;
    }

    Utils::Check << device->CreateTexture2D(&textureDesc, pInitData, _texture.GetAddressOf());

    if (BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format =
            BindFlags & D3D11_BIND_DEPTH_STENCIL
            ? GetDepthSRVFormat(Format)
            : Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = Mips;
        Utils::Check << device->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf());
    }

    if (BindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (uint32_t face = 0; face < 6; face++) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = GetDSVFormat(Format);
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = face;
            dsvDesc.Texture2DArray.ArraySize = 1;

            Utils::Check << device->CreateDepthStencilView(_texture.Get(), &dsvDesc, _cubemapDSVs[face].GetAddressOf());
        }
    }

    if (BindFlags & D3D11_BIND_RENDER_TARGET) {
        for (uint32_t face = 0; face < 6; face++) {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = Format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;

            for (int mip = 0; mip < Mips; ++mip) {
                rtvDesc.Texture2DArray.MipSlice = mip;
                Utils::Check << device->CreateRenderTargetView(_texture.Get(), &rtvDesc, _cubemapMipRTVs[face][mip].GetAddressOf());
            }
        }
    }

}

auto BeTexture::CreateMipViewports() -> void {
    _mipViewports.resize(Mips);
    for (int i = 0; i < Mips; ++i) {
        auto& viewport = _mipViewports[i];
        viewport.Width = static_cast<float>(Width >> i);
        viewport.Height = static_cast<float>(Height >> i);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
    }
}

auto BeTexture::GetDepthSRVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> textureToSRV = {
        {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS},
        {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM},
    };
    return textureToSRV.at(textureFormat);
}

auto BeTexture::GetDSVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT {
    static std::unordered_map<DXGI_FORMAT, DXGI_FORMAT> textureToDSV = {
        {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT},
        {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT},
        {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM},
    };
    return textureToDSV.at(textureFormat);
}
