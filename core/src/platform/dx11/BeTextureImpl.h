#pragma once

#include <array>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeTextureImpl {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11DepthStencilView> dsv;
    std::vector<ComPtr<ID3D11RenderTargetView>> mipRTVs;

    std::array<ComPtr<ID3D11DepthStencilView>, 6> cubemapDSVs;
    std::array<std::vector<ComPtr<ID3D11RenderTargetView>>, 6> cubemapMipRTVs;
};
