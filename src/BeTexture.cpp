#include "BeTexture.h"

#include <stb_image.h>
#include "Utils.h"

auto BeTexture::CreateFromFile(const std::filesystem::path& path, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture> {
    int w = 0, h = 0, channelsInFile = 0;
    uint8_t* decoded = stbi_load(path.string().c_str(), &w, &h, &channelsInFile, 4);
    if (!decoded) throw std::runtime_error("Failed to load texture from file: " + path.string());

    const size_t imageSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    auto pixels = static_cast<uint8_t*>(malloc(imageSize));
    if (!pixels) {
        stbi_image_free(decoded);
        throw std::runtime_error("Failed to allocate texture");
    }
    memcpy(pixels, decoded, imageSize);
    stbi_image_free(decoded);

    auto texture = std::make_shared<BeTexture>();
    texture->Pixels = pixels;
    texture->Width = w;
    texture->Height = h;
    texture->FlipVertically();
    texture->CreateSRV(device);
    return texture;
}

auto BeTexture::CreateFromColor(const glm::vec4& color, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture> {
    auto texture = std::make_shared<BeTexture>();
    texture->Width = 1;
    texture->Height = 1;
    texture->Pixels = static_cast<uint8_t*>(malloc(4));
    if (!texture->Pixels) throw std::runtime_error("Failed to allocate texture");
    texture->Pixels[0] = static_cast<uint8_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
    texture->Pixels[1] = static_cast<uint8_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
    texture->Pixels[2] = static_cast<uint8_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
    texture->Pixels[3] = static_cast<uint8_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
    texture->CreateSRV(device);
    return texture;
}

auto BeTexture::CreateFromMemory(const uint8_t* data, uint32_t width, uint32_t height, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture> {
    const size_t count = width * height;
    if (count == 0) throw std::runtime_error("Invalid texture dimensions");

    auto pixels = static_cast<uint8_t*>(malloc(count * 4));
    if (!pixels) throw std::runtime_error("Failed to allocate texture");

    const uint8_t* src = data;
    for (size_t i = 0; i < count; ++i) {
        pixels[4 * i + 0] = src[4 * i + 0]; // r
        pixels[4 * i + 1] = src[4 * i + 1]; // g
        pixels[4 * i + 2] = src[4 * i + 2]; // b
        pixels[4 * i + 3] = src[4 * i + 3]; // a
    }

    auto texture = std::make_shared<BeTexture>();
    texture->Pixels = pixels;
    texture->Width = width;
    texture->Height = height;
    texture->FlipVertically();
    texture->CreateSRV(device);
    return texture;
}

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
