#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

using Microsoft::WRL::ComPtr;

struct ISlangBlob;
namespace Slang { template <typename T> class ComPtr; }

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

struct SenDx11ShaderEntry {
    ComPtr<ID3D11DeviceChild> Shader; // polymorphic: ID3D11VertexShader, ID3D11PixelShader, ID3D11HullShader, or ID3D11DomainShader
    Slang::ComPtr<ISlangBlob>* SlangBlobPtr = nullptr;
};


struct SenDx11PipelineEntry {
    // Shader objects
    ComPtr<ID3D11VertexShader>   VertexShader;
    ComPtr<ID3D11HullShader>     HullShader;
    ComPtr<ID3D11DomainShader>   DomainShader;
    ComPtr<ID3D11PixelShader>    PixelShader;

    // Vertex input
    ComPtr<ID3D11InputLayout>    InputLayout;
    D3D11_PRIMITIVE_TOPOLOGY     Topology;

    // Render state
    ComPtr<ID3D11RasterizerState>   RasterizerState;
    ComPtr<ID3D11BlendState>        BlendState;
    ComPtr<ID3D11DepthStencilState> DepthStencilState;
};

// ─── backend ──────────────────────────────────────────────────────────────────

class SenDx11Backend {

    hide
    static SenDx11Backend* _instance;

    ComPtr<ID3D11Device>        _device;
    ComPtr<ID3D11DeviceContext> _context;

    std::unordered_map<uint32_t, SenDx11TextureEntry> _textures;
    uint32_t _nextTextureId = 1;

    std::unordered_map<uint32_t, SenDx11BufferEntry> _buffers;
    uint32_t _nextBufferId = 1;

    std::unordered_map<uint32_t, SenDx11SamplerEntry> _samplers;
    uint32_t _nextSamplerId = 1;

    std::unordered_map<uint32_t, SenDx11ShaderEntry> _shaders;
    uint32_t _nextShaderId = 1;

    std::unordered_map<uint32_t, SenDx11PipelineEntry> _pipelines;
    uint32_t _nextPipelineId = 1;

    expose
    static auto Init     (const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context) -> void;
    static auto Shutdown () -> void;
    static auto Get      () -> SenDx11Backend&;

    expose // textures
    auto CreateTexture  (const SenTextureDesc& desc) -> SenTexture;
    auto DestroyTexture (SenTexture handle) -> void;
    auto LookupTexture  (SenTexture handle) -> SenDx11TextureEntry&;

    expose // buffers
    auto CreateBuffer  (const SenBufferDesc& desc) -> SenBuffer;
    auto DestroyBuffer (SenBuffer handle) -> void;
    auto LookupBuffer  (SenBuffer handle) -> SenDx11BufferEntry&;
    auto WriteBuffer   (SenBuffer handle, const void* data, uint32_t size) -> void;

    expose // samplers
    auto CreateSampler  (const SenSamplerDesc& desc) -> SenSampler;
    auto DestroySampler (SenSampler handle) -> void;
    auto LookupSampler  (SenSampler handle) -> SenDx11SamplerEntry&;

    expose // shaders
    auto CreateShader (const SenShaderSourceDesc& sourceDesc) -> SenShader;
    auto DestroyShader (SenShader handle) -> void;
    auto LookupShader  (SenShader handle) -> SenDx11ShaderEntry&;

    expose // pipelines
    auto CreatePipeline (const SenPipelineDesc& desc) -> SenPipeline;
    auto DestroyPipeline (SenPipeline handle) -> void;
    auto LookupPipeline  (SenPipeline handle) -> SenDx11PipelineEntry&;

};
