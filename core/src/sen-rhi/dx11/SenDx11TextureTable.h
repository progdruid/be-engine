#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTexture.h>

using Microsoft::WRL::ComPtr;

struct SenDx11TextureEntry {
    ComPtr<ID3D11Texture2D>                                    Texture;
    ComPtr<ID3D11ShaderResourceView>                           SRV;
    ComPtr<ID3D11DepthStencilView>                             DSV;
    std::vector<ComPtr<ID3D11RenderTargetView>>                MipRTVs;
    std::array<ComPtr<ID3D11DepthStencilView>, 6>              CubemapDSVs;
    std::array<std::vector<ComPtr<ID3D11RenderTargetView>>, 6> CubemapMipRTVs;
};

class SenDx11TextureTable {

    hide static SenDx11TextureTable* _instance;
    hide std::unordered_map<uint32_t, SenDx11TextureEntry> _entries;
    hide uint32_t _nextId = 1;

    expose static auto Init     () -> void;
    expose static auto Shutdown () -> void;
    expose static auto Get      () -> SenDx11TextureTable&;

    expose auto Create  (const ComPtr<ID3D11Device>& device, const SenTextureDesc& desc) -> SenTexture;
    expose auto Destroy (SenTexture handle) -> void;
    expose auto Lookup  (SenTexture handle) -> SenDx11TextureEntry&;
};
