#include "BeShader.h"
#include <d3dcompiler.h>
#include <format>
#include "Utils.h"

BeShader::BeShader(
    ID3D11Device* device,
    const std::wstring& filePathWithoutExtension,
    const std::vector<BeVertexElementDescriptor>& vertexLayout)
        : VertexLayout(vertexLayout) {

    
    
    //shaders
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
            Utils::ThrowIfFailed(hr);
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
            Utils::ThrowIfFailed(hr);
        }
    }

    Utils::ThrowIfFailed(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader));
    Utils::ThrowIfFailed(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader));


    //input layout
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
    inputLayout.reserve(vertexLayout.size());

    for (const auto& descriptor : vertexLayout) {
        D3D11_INPUT_ELEMENT_DESC elementDesc;
        elementDesc.SemanticIndex = 0;
        elementDesc.InputSlot = 0;
        elementDesc.AlignedByteOffset = BeVertexElementDescriptor::ElementOffsets.at(descriptor.Attribute);
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;
        elementDesc.SemanticName = BeVertexElementDescriptor::SemanticNames.at(descriptor.Attribute);
        elementDesc.Format = BeVertexElementDescriptor::ElementFormats.at(descriptor.Attribute);
        
        inputLayout.push_back(elementDesc);
    }

    Utils::ThrowIfFailed(device->CreateInputLayout(
        inputLayout.data(),
        static_cast<UINT>(inputLayout.size()),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &ComputedInputLayout));
}

auto BeShader::bind(ID3D11DeviceContext* context) const -> void {
    context->IASetInputLayout(ComputedInputLayout.Get());
    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);
}

