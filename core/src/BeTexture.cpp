#include "BeTexture.h"

#include <umbrellas/include-glm.h>
#include <stb_image/stb_image.h>

#include "BeAssetRegistry.h"
#include <sen-rhi/dx11/SenDx11Backend.h>


BeTexture::Builder::Builder(std::string name) { _descriptor.Name = std::move(name); }

BeTexture::Builder::~Builder() {
    if (_descriptor.Data) {
        free(_descriptor.Data);
        _descriptor.Data = nullptr;
    }
}

auto BeTexture::Builder::SetUsage  (SenTextureUsage usage)  -> Builder&& { _descriptor.Usage = usage;   return std::move(*this); }
auto BeTexture::Builder::SetFormat (SenFormat format)        -> Builder&& { _descriptor.Format = format; return std::move(*this); }
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

        memcpy(tempRow, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, tempRow, rowSize);
    }

    delete[] tempRow;
}

auto BeTexture::Builder::AddToRegistry() -> Builder&& { _addToRegistry = true; return std::move(*this); }


auto BeTexture::Builder::Build(const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture> {
    std::shared_ptr<BeTexture> resource(new BeTexture(device, _descriptor));
    if (_addToRegistry)
        BeAssetRegistry::AddTexture(_descriptor.Name, resource);
    return resource;
}

auto BeTexture::Builder::BuildNoReturn(const ComPtr<ID3D11Device>& device) -> void {
    const std::shared_ptr<BeTexture> resource(new BeTexture(device, _descriptor));
    if (_addToRegistry)
        BeAssetRegistry::AddTexture(_descriptor.Name, resource);
}


BeTexture::BeTexture(ComPtr<ID3D11Device> device, const BeTextureDescriptor& descriptor)
: Name(descriptor.Name)
, Width(descriptor.Width)
, Height(descriptor.Height)
, IsCubemap(descriptor.IsCubemap)
, Mips(descriptor.Mips)
, Usage(descriptor.Usage)
, Format(descriptor.Format)
{
    SenTextureDesc senDesc;
    senDesc.Format  = descriptor.Format;
    senDesc.Width   = descriptor.Width;
    senDesc.Height  = descriptor.Height;
    senDesc.Usage   = descriptor.Usage;
    senDesc.Mips    = descriptor.Mips;
    senDesc.Cubemap = descriptor.IsCubemap;
    senDesc.Data    = descriptor.Data;

    Handle = SenDx11Backend::Get().CreateTexture(senDesc);

    CreateMipViewports();
}

BeTexture::~BeTexture() {
    SenDx11Backend::Get().DestroyTexture(Handle);
}


auto BeTexture::GetMipViewport(const uint32_t mip) const -> const SenViewport& { return _mipViewports[mip]; }


auto BeTexture::CreateMipViewports() -> void {
    _mipViewports.resize(Mips);
    for (uint32_t i = 0; i < Mips; ++i) {
        auto& viewport = _mipViewports[i];
        viewport.Width    = static_cast<float>(Width >> i);
        viewport.Height   = static_cast<float>(Height >> i);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.X        = 0.0f;
        viewport.Y        = 0.0f;
    }
}
