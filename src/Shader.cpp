#include "Shader.h"
#include <d3dcompiler.h>
#include <fstream>
#include "DXUtils.h"

Shader::Shader(
    ID3D11Device* device,
    const std::wstring& filePathWithoutExtension,
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDesc) {

    const std::wstring vsPath = filePathWithoutExtension + L"Vertex.hlsl";
    const std::wstring psPath = filePathWithoutExtension + L"Pixel.hlsl";

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    auto hr = D3DCompileFromFile(
        vsPath.c_str(),
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        0, 0,
        &vsBlob,
        &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            std::string errorMsg(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
            throw std::runtime_error("Vertex shader compilation error: " + errorMsg);
        } else {
            DX::ThrowIfFailed(hr);
        }
    }
    
    hr = D3DCompileFromFile(
        psPath.c_str(),
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        0, 0,
        &psBlob,
        &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            std::string errorMsg(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
            throw std::runtime_error("Vertex shader compilation error: " + errorMsg);
        } else {
            DX::ThrowIfFailed(hr);
        }
    }

    DX::ThrowIfFailed(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader));
    DX::ThrowIfFailed(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader));
    DX::ThrowIfFailed(device->CreateInputLayout(
        inputLayoutDesc.data(),
        static_cast<UINT>(inputLayoutDesc.size()),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &InputLayout));
}

auto Shader::bind(ID3D11DeviceContext* context) const -> void {
    context->IASetInputLayout(InputLayout.Get());
    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);
}
