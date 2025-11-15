#pragma once
#include <cstdint>
#include <wrl/client.h>
#include <d3d11.h>
#include <filesystem>
#include <umbrellas/include-glm.h>

using Microsoft::WRL::ComPtr;

struct BeTexture {
    // Static
    static auto CreateFromFile(const std::filesystem::path& path, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture>;
    static auto CreateFromColor(const glm::vec4& color, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture>;
    static auto CreateFromMemory(const uint8_t* data, uint32_t width, uint32_t height, const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture>;

    uint8_t* Pixels = nullptr; // RGBA8
    uint32_t Width = 0;
    uint32_t Height = 0;
    ComPtr<ID3D11ShaderResourceView> SRV = nullptr;

    BeTexture() = default;
    ~BeTexture() { free(Pixels); }

    auto FlipVertically() -> void;
    auto CreateSRV(const ComPtr<ID3D11Device>& device) -> void;

private:
    explicit BeTexture(const glm::vec4& color);
};
