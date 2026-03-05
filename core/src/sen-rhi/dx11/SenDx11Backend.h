#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTexture.h>
#include <sen-rhi/SenBuffer.h>
#include <sen-rhi/SenSampler.h>

using Microsoft::WRL::ComPtr;

// ─── resource entries ─────────────────────────────────────────────────────────

struct SenDx11TextureEntry {
    ComPtr<ID3D11Texture2D>                                    Texture;
    ComPtr<ID3D11ShaderResourceView>                           SRV;
    ComPtr<ID3D11DepthStencilView>                             DSV;
    std::vector<ComPtr<ID3D11RenderTargetView>>                MipRTVs;
    std::array<ComPtr<ID3D11DepthStencilView>, 6>              CubemapDSVs;
    std::array<std::vector<ComPtr<ID3D11RenderTargetView>>, 6> CubemapMipRTVs;
};

struct SenDx11BufferEntry {
    ComPtr<ID3D11Buffer> Buffer;
    SenBufferAccess      Access;
};

struct SenDx11SamplerEntry {
    ComPtr<ID3D11SamplerState> Sampler;
};

// ─── backend ──────────────────────────────────────────────────────────────────

class SenDx11Backend {

    hide static SenDx11Backend* _instance;

    hide std::unordered_map<uint32_t, SenDx11TextureEntry> _textures;
    hide uint32_t _nextTextureId = 1;

    hide std::unordered_map<uint32_t, SenDx11BufferEntry> _buffers;
    hide uint32_t _nextBufferId = 1;

    hide std::unordered_map<uint32_t, SenDx11SamplerEntry> _samplers;
    hide uint32_t _nextSamplerId = 1;

    expose static auto Init     () -> void;
    expose static auto Shutdown () -> void;
    expose static auto Get      () -> SenDx11Backend&;

    // textures
    expose auto CreateTexture  (const ComPtr<ID3D11Device>& device, const SenTextureDesc& desc) -> SenTexture;
    expose auto DestroyTexture (SenTexture handle) -> void;
    expose auto LookupTexture  (SenTexture handle) -> SenDx11TextureEntry&;

    // buffers
    expose auto CreateBuffer  (const ComPtr<ID3D11Device>& device, const SenBufferDesc& desc) -> SenBuffer;
    expose auto DestroyBuffer (SenBuffer handle) -> void;
    expose auto LookupBuffer  (SenBuffer handle) -> SenDx11BufferEntry&;
    expose auto WriteBuffer   (SenBuffer handle, const void* data, uint32_t size, const ComPtr<ID3D11DeviceContext>& context) -> void;

    // samplers
    expose auto CreateSampler  (const ComPtr<ID3D11Device>& device, const SenSamplerDesc& desc) -> SenSampler;
    expose auto DestroySampler (SenSampler handle) -> void;
    expose auto LookupSampler  (SenSampler handle) -> SenDx11SamplerEntry&;
};
