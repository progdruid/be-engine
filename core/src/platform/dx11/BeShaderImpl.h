#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeShaderImpl {
    ComPtr<ID3D11InputLayout> computedInputLayout;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11HullShader> hullShader;
    ComPtr<ID3D11DomainShader> domainShader;
    ComPtr<ID3D11PixelShader> pixelShader;
};
