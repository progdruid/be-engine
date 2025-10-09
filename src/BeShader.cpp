#include "BeShader.h"
#include <d3dcompiler.h>
#include <format>
#include "Utils.h"
#include "BeShaderIncludeHandler.hpp"

BeShader::BeShader(
    ID3D11Device* device,
    const std::filesystem::path& filePathWithoutExtension,
    const std::vector<BeVertexElementDescriptor>& vertexLayout)
        : VertexLayout(vertexLayout) {

    BeShaderIncludeHandler includeHandler(
        filePathWithoutExtension.parent_path().string(),
        "src/shaders/"
    );
    
    //shaders
    std::filesystem::path vsPath = filePathWithoutExtension;
    std::filesystem::path psPath = filePathWithoutExtension;
    vsPath += "Vertex.hlsl";
    psPath += "Pixel.hlsl";

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    auto hr = D3DCompileFromFile(
        vsPath.wstring().c_str(),
        nullptr,
        &includeHandler,
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
        psPath.wstring().c_str(),
        nullptr,
        &includeHandler,
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

    Utils::Check << device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader);
    Utils::Check << device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader);

    if (vertexLayout.empty()) {
        return;
    }
    
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

    Utils::Check << device->CreateInputLayout(
        inputLayout.data(),
        static_cast<UINT>(inputLayout.size()),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &ComputedInputLayout);
}

auto BeShader::Bind(ID3D11DeviceContext* context) const -> void {
    context->IASetInputLayout(ComputedInputLayout.Get());
    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);
}

