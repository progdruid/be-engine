#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <vector>
using Microsoft::WRL::ComPtr;

class Shader {
public:
    Shader(ID3D11Device* device, const std::wstring& filePathWithoutExtension,
           const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDesc);
    ~Shader() = default;

    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;

    auto bind (ID3D11DeviceContext* context) const -> void;
};
