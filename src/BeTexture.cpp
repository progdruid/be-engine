#include "BeTexture.h"

#include "Utils.h"


BeTexture::BeTexture(const glm::vec4& color) {
    Width = 1;
    Height = 1;
    Pixels = static_cast<uint8_t*>(malloc(4));
    if (!Pixels) throw std::runtime_error("Failed to allocate texture");
    Pixels[0] = static_cast<uint8_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
    Pixels[1] = static_cast<uint8_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
    Pixels[2] = static_cast<uint8_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
    Pixels[3] = static_cast<uint8_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
}


// ReSharper disable once CppMemberFunctionMayBeConst
auto BeTexture::FlipVertically() -> void {
    const uint32_t rowSize = Width * 4; // 4 bytes per pixel (RGBA8)
    const auto tempRow = new uint8_t[rowSize];

    for (uint32_t y = 0; y < Height / 2; ++y) {
        uint8_t* topRow = Pixels + y * rowSize;
        uint8_t* bottomRow = Pixels + (Height - 1 - y) * rowSize;

        // Swap topRow and bottomRow
        memcpy(tempRow, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow, rowSize);
    }

    delete[] tempRow;
}

auto BeTexture::CreateSRV(const ComPtr<ID3D11Device>& device) -> void {
    const D3D11_TEXTURE2D_DESC desc = {
        .Width = Width,
        .Height = Height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1 },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0, 
    };

    const D3D11_SUBRESOURCE_DATA initData = {
        .pSysMem = Pixels,
        .SysMemPitch = 4 * Width
    };
            
    ComPtr<ID3D11Texture2D> d3dTexture = nullptr;
    Utils::Check << device->CreateTexture2D(&desc, &initData, &d3dTexture);
            
    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1 },
    };
    Utils::Check << device->CreateShaderResourceView(d3dTexture.Get(), &srvDescriptor, SRV.GetAddressOf());
}
