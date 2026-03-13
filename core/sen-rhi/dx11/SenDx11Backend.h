#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <umbrellas/access-modifiers.hpp>
#include <sen-rhi/SenTypes.h>

#include "BeRenderer.h"
#include "SenDx11CommandBuffer.h"
#include "sen-rhi/SenCommandBuffer.h"

using Microsoft::WRL::ComPtr;

struct ISlangBlob;
struct ID3DUserDefinedAnnotation;
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

struct SenDx11SwapchainEntry {
    ComPtr<IDXGISwapChain1> Swapchain;
    ComPtr<ID3D11RenderTargetView> BackbufferRTV;
    SenTexture BackbufferTextureHandle;
    uint32_t Width;
    uint32_t Height;
    SenFormat Format;
};

// ─── backend ──────────────────────────────────────────────────────────────────

class SenDx11Backend {
    expose
    static auto Init     (const SenDeviceDesc& desc) -> void;
    static auto Shutdown () -> void;

    
    expose // swapchain lifecycle
    static auto CreateSwapchain  (const SenSwapchainDesc& desc) -> SenSwapchain;
    static auto DestroySwapchain (SenSwapchain handle) -> void;
    static auto ResizeSwapchain  (SenSwapchain& handle, uint32_t width, uint32_t height) -> void;
    static auto GetSwapchainFormat(SenSwapchain swapchain) -> SenFormat;
    static auto BeginFrame       (SenSwapchain handle) -> SenTexture;
    static auto EndFrame         (SenSwapchain handle) -> void;

    expose // command buffer factory
    static auto CreateCommandBuffer () -> SenDx11CommandBuffer;

    expose // native API escape hatches (for ImGui, etc.)
    static auto GetNativeDevice  () -> void*;
    static auto GetNativeContext () -> void*;

    expose // debug annotation helpers
    static auto BeginDebugEvent (const std::string& label) -> void;
    static auto EndDebugEvent   () -> void;
    
    
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

    expose // bind groups
    static auto CreateBindGroup  (const SenBindGroupDesc& desc) -> SenBindGroup;
    static auto DestroyBindGroup (SenBindGroup handle) -> void;
    static auto LookupBindGroup  (SenBindGroup handle) -> SenBindGroupDesc&;

    
    expose // shaders
    static auto CreateShader (const SenShaderSourceDesc& sourceDesc) -> SenShader;
    static auto DestroyShader (SenShader handle) -> void;
    static auto LookupShader  (SenShader handle) -> SenDx11ShaderEntry&;

    expose // pipelines
    static auto CreatePipeline (const SenPipelineDesc& desc) -> SenPipeline;
    static auto DestroyPipeline (SenPipeline handle) -> void;
    static auto LookupPipeline  (SenPipeline handle) -> SenDx11PipelineEntry&;


    hide
    static ComPtr<ID3D11Device>        _device;
    static ComPtr<ID3D11DeviceContext> _context;
    static ComPtr<IDXGIFactory2>       _factory;
    static ComPtr<ID3D11DepthStencilState> _defaultDepthStencilState;
    static ComPtr<ID3D11RasterizerState>   _rasterizerCullBack;
    static ComPtr<ID3D11RasterizerState>   _rasterizerCullNone;
    static ComPtr<ID3DUserDefinedAnnotation> _annotation;
    
    static std::unordered_map<uint32_t, SenDx11SwapchainEntry> _swapchains;
    static uint32_t _nextSwapchainId;
    
    static std::unordered_map<uint32_t, SenDx11TextureEntry> _textures;
    static uint32_t _nextTextureId;

    static std::unordered_map<uint32_t, SenDx11BufferEntry> _buffers;
    static uint32_t _nextBufferId;

    static std::unordered_map<uint32_t, SenDx11SamplerEntry> _samplers;
    static uint32_t _nextSamplerId;

    static std::unordered_map<uint32_t, SenBindGroupDesc> _bindGroups;
    static uint32_t _nextBindGroupId;

    static std::unordered_map<uint32_t, SenDx11ShaderEntry> _shaders;
    static uint32_t _nextShaderId;

    static std::unordered_map<uint32_t, SenDx11PipelineEntry> _pipelines;
    static uint32_t _nextPipelineId;

    hide
    static auto GetBestAdapter() -> ComPtr<IDXGIAdapter1>;
};
