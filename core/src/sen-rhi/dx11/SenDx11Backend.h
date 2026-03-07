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
    expose
    static auto Init     (const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context) -> void;
    static auto Shutdown () -> void;

    expose // textures
    static auto CreateTexture  (const SenTextureDesc& desc) -> SenTexture;
    static auto DestroyTexture (SenTexture handle) -> void;
    static auto LookupTexture  (SenTexture handle) -> SenDx11TextureEntry&;

    expose // buffers
    static auto CreateBuffer  (const SenBufferDesc& desc) -> SenBuffer;
    static auto DestroyBuffer (SenBuffer handle) -> void;
    static auto LookupBuffer  (SenBuffer handle) -> SenDx11BufferEntry&;
    static auto WriteBuffer   (SenBuffer handle, const void* data, uint32_t size) -> void;

    expose // samplers
    static auto CreateSampler  (const SenSamplerDesc& desc) -> SenSampler;
    static auto DestroySampler (SenSampler handle) -> void;
    static auto LookupSampler  (SenSampler handle) -> SenDx11SamplerEntry&;

    expose // shaders
    static auto CreateShader (const SenShaderSourceDesc& sourceDesc) -> SenShader;
    static auto DestroyShader (SenShader handle) -> void;
    static auto LookupShader  (SenShader handle) -> SenDx11ShaderEntry&;

    expose // pipelines
    static auto CreatePipeline (const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline (SenPipeline handle) -> void;
    static auto LookupPipeline  (SenPipeline handle) -> SenDx11PipelineEntry&;

    expose // render passes
    static auto RegisterBackbuffer(const ComPtr<ID3D11RenderTargetView>& backbufferRTV) -> SenTexture;

    expose // bind groups
    static auto CreateBindGroup  (const SenBindGroupDesc& desc) -> SenBindGroup;
    static auto DestroyBindGroup (SenBindGroup handle) -> void;
    static auto LookupBindGroup  (SenBindGroup handle) -> SenBindGroupDesc&;

    hide
    static ComPtr<ID3D11Device>        _device;
    static ComPtr<ID3D11DeviceContext> _context;

    static std::unordered_map<uint32_t, SenDx11TextureEntry> _textures;
    static uint32_t _nextTextureId;

    static std::unordered_map<uint32_t, SenDx11BufferEntry> _buffers;
    static uint32_t _nextBufferId;

    static std::unordered_map<uint32_t, SenDx11SamplerEntry> _samplers;
    static uint32_t _nextSamplerId;

    static std::unordered_map<uint32_t, SenDx11ShaderEntry> _shaders;
    static uint32_t _nextShaderId;

    static std::unordered_map<uint32_t, SenDx11PipelineEntry> _pipelines;
    static uint32_t _nextPipelineId;

    static std::unordered_map<uint32_t, SenBindGroupDesc> _bindGroups;
    static uint32_t _nextBindGroupId;
};
