#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeMaterialImpl {
    ComPtr<ID3D11Buffer> cbuffer;
    ComPtr<ID3D11DeviceContext> context;
    bool isFrequentlyUsed = false;
};
