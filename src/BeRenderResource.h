#pragma once
#include <array>
#include <cstdint>
#include <wrl/client.h>
#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <glm/vec4.hpp>

#include "umbrellas/access-modifiers.hpp"

using Microsoft::WRL::ComPtr;

class BeRenderResource {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct BeResourceDescriptor {
        bool IsCubemap = false;
        DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        uint32_t Mips = 1;
        uint32_t CustomWidth = 0;
        uint32_t CustomHeight = 0;
        glm::vec4 DefaultColor = glm::vec4(0.0f);
    };

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create (ComPtr<ID3D11Device> device, std::string name, const BeResourceDescriptor& descriptor) -> std::shared_ptr<BeRenderResource>;
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose BeResourceDescriptor Descriptor;
    
    hide std::vector<D3D11_VIEWPORT> _mipViewports;
    
    hide ComPtr<ID3D11Texture2D> _texture;
    hide ComPtr<ID3D11ShaderResourceView> _srv;
    hide ComPtr<ID3D11DepthStencilView> _dsv;
    hide std::vector<ComPtr<ID3D11RenderTargetView>> _mipRTVs;

    hide std::array<ComPtr<ID3D11DepthStencilView>, 6> _cubemapDSVs;
    hide std::array<std::vector<ComPtr<ID3D11RenderTargetView>>, 6> _cubemapMipRTVs;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose explicit BeRenderResource(std::string name, const BeResourceDescriptor& descriptor);
    expose ~BeRenderResource();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto CreateGPUResources(ComPtr<ID3D11Device> device) -> void;
    
    expose auto GetMipViewport (const uint32_t mip) const              -> const D3D11_VIEWPORT&;
    expose auto GetSRV         () const                                -> ComPtr<ID3D11ShaderResourceView>;
    expose auto GetDSV         () const                                -> ComPtr<ID3D11DepthStencilView>;
    expose auto GetRTV         (const uint32_t mip = 0) const          -> ComPtr<ID3D11RenderTargetView>;
    expose auto GetCubemapDSV  (uint32_t faceIndex)                    -> ComPtr<ID3D11DepthStencilView>;
    expose auto GetCubemapRTV  (uint32_t faceIndex, uint32_t mip = 0)  -> ComPtr<ID3D11RenderTargetView>;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto CreateTexture2DResources (ComPtr<ID3D11Device> device) -> void;
    hide auto CreateCubemapResources   (ComPtr<ID3D11Device> device) -> void;
    hide auto CreateMipViewports () -> void;

    hide auto GetDepthSRVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT;
    hide auto GetDSVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT;


    // befriending shared_ptr for constructor/destructor access because ours are private
    friend class std::shared_ptr<BeRenderResource>;
};

