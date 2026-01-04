#pragma once
#include <array>
#include <cstdint>
#include <wrl/client.h>
#include <d3d11.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

using Microsoft::WRL::ComPtr;

class BeTexture {

    // types ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose struct BeTextureDescriptor {
        std::string Name;
        bool IsCubemap = false;
        DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t BindFlags = D3D11_BIND_SHADER_RESOURCE;
        uint32_t Mips = 1;
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint8_t* Data = nullptr;
    };
    
    expose class Builder {

        hide BeTextureDescriptor _descriptor;
        hide bool _addToRegistry = false;

        hide explicit Builder (std::string name);
        expose ~Builder ();
        expose Builder (const Builder&) = delete;
        expose auto operator=(const Builder&) -> Builder& = delete;
        expose Builder (Builder&&) = default;
        expose auto operator=(Builder&&) -> Builder& = default;

        expose auto SetBindFlags(uint32_t bindFlags) -> Builder&& ;
        expose auto SetFormat(DXGI_FORMAT format) -> Builder&& ;
        expose auto SetMips(uint32_t mips) -> Builder&&;
        expose auto SetSize(uint32_t w, uint32_t h) -> Builder&& ;
        expose auto SetCubemap(bool cubemap) -> Builder&& ;

        expose auto FillWithColor (const glm::vec4& color) -> Builder&&;
        expose auto FillFromMemory (const uint8_t* src) -> Builder&&;
        expose auto LoadFromFile (const std::filesystem::path& file) -> Builder&&;

        hide static auto FlipVertically (uint32_t w, uint32_t h, uint8_t* data) -> void;

        expose auto AddToRegistry () -> Builder&&;

        expose auto Build(const ComPtr<ID3D11Device>& device) -> std::shared_ptr<BeTexture>;
        expose auto BuildNoReturn(const ComPtr<ID3D11Device>& device) -> void;

        friend class BeTexture;
    }; 

    // static part /////////////////////////////////////////////////////////////////////////////////////////////////////
    expose static auto Create (std::string name) -> Builder { return Builder (std::move(name)); }
    
    // fields //////////////////////////////////////////////////////////////////////////////////////////////////////////
    expose std::string Name;
    expose uint32_t UniqueID;
    expose uint32_t Width;
    expose uint32_t Height;
    expose bool IsCubemap;
    expose uint32_t Mips;
    expose uint32_t BindFlags;
    expose DXGI_FORMAT Format;
    
    hide std::vector<D3D11_VIEWPORT> _mipViewports;
    
    hide ComPtr<ID3D11Texture2D> _texture;
    hide ComPtr<ID3D11ShaderResourceView> _srv;
    hide ComPtr<ID3D11DepthStencilView> _dsv;
    hide std::vector<ComPtr<ID3D11RenderTargetView>> _mipRTVs;

    hide std::array<ComPtr<ID3D11DepthStencilView>, 6> _cubemapDSVs;
    hide std::array<std::vector<ComPtr<ID3D11RenderTargetView>>, 6> _cubemapMipRTVs;

    // lifetime ////////////////////////////////////////////////////////////////////////////////////////////////////////
    hide explicit BeTexture(ComPtr<ID3D11Device> device, const BeTextureDescriptor& descriptor);
    expose ~BeTexture();

    // public interface ////////////////////////////////////////////////////////////////////////////////////////////////
    expose auto GetMipViewport (const uint32_t mip) const              -> const D3D11_VIEWPORT&;
    expose auto GetSRV         () const                                -> ComPtr<ID3D11ShaderResourceView>;
    expose auto GetDSV         () const                                -> ComPtr<ID3D11DepthStencilView>;
    expose auto GetRTV         (const uint32_t mip = 0) const          -> ComPtr<ID3D11RenderTargetView>;
    expose auto GetCubemapDSV  (uint32_t faceIndex)                    -> ComPtr<ID3D11DepthStencilView>;
    expose auto GetCubemapRTV  (uint32_t faceIndex, uint32_t mip = 0)  -> ComPtr<ID3D11RenderTargetView>;

    // private logic ///////////////////////////////////////////////////////////////////////////////////////////////////
    hide auto CreateTexture2DResources (ComPtr<ID3D11Device> device, const uint8_t* defaultData = nullptr) -> void;
    hide auto CreateCubemapResources   (ComPtr<ID3D11Device> device, const uint8_t* defaultData = nullptr) -> void;
    hide auto CreateMipViewports () -> void;

    hide auto GetDepthSRVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT;
    hide auto GetDSVFormat(DXGI_FORMAT textureFormat) const -> DXGI_FORMAT;


    // befriending shared_ptr for constructor/destructor access because ours are private
    friend class std::shared_ptr<BeTexture>;
};

