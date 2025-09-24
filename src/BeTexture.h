#pragma once
#include <cstdint>
#include <wrl/client.h>
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

struct BeTexture {
    inline ~BeTexture() { free (Pixels); }

    uint8_t* Pixels = nullptr; // RGBA8
    uint32_t Width = 0;
    uint32_t Height = 0;
    ComPtr<ID3D11ShaderResourceView> SRV = nullptr;
};
