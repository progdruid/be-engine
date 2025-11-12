#include "BeShader.h"

#include <cassert>
#include <d3dcompiler.h>
#include "BeShaderIncludeHandler.hpp"
#include "Utils.h"

BeShader::BeShader(ID3D11Device* device, const std::filesystem::path& filePath) {
    BeShaderIncludeHandler includeHandler(
        filePath.parent_path().string(),
        "src/shaders/"
    );

    std::filesystem::path path = filePath;
    path += ".hlsl";
    assert(std::filesystem::exists(path));
    
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string src = buffer.str();
    const Json header = ParseHeader(src);

    std::string smth = header.dump(4);
    
    if (header.contains("vertex")) {
        _shaderType = BeShaderType::Vertex;

        std::string vertexFunctionName = header["vertex"];
        ComPtr<ID3DBlob> blob = CompileBlob(path, vertexFunctionName.c_str(), "vs_5_0", &includeHandler);
        Utils::Check << device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &VertexShader);
        
        //input layout
        if (header.contains("vertexLayout")) {
            Json vertexLayoutJson = header["vertexLayout"];
            
            std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
            inputLayout.reserve(vertexLayoutJson.size());

            for (const auto& descriptorJson : vertexLayoutJson) {
                BeVertexElementDescriptor descriptor;
                descriptor.Attribute = BeVertexElementDescriptor::SemanticMap.at(descriptorJson);
                
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
                blob->GetBufferPointer(),
                blob->GetBufferSize(),
                &ComputedInputLayout);
        }
    }

    if (header.contains("pixel")) {
        _shaderType = _shaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header["pixel"];
        ComPtr<ID3DBlob> blob = CompileBlob(path, pixelFunctionName.c_str(), "ps_5_0", &includeHandler);
        Utils::Check << device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &PixelShader);
    }
    
}

auto BeShader::Bind(ID3D11DeviceContext* context) const -> void {
    if (HasAny(_shaderType, BeShaderType::Vertex)) {
        context->IASetInputLayout(ComputedInputLayout.Get());
        context->VSSetShader(VertexShader.Get(), nullptr, 0);
    }
    if (HasAny(_shaderType, BeShaderType::Pixel)) {
        context->PSSetShader(PixelShader.Get(), nullptr, 0);
    }
}

auto BeShader::CompileBlob(
    const std::filesystem::path& filePath,
    const char* entrypointName,
    const char* target,
    BeShaderIncludeHandler* includeHandler)
-> ComPtr<ID3DBlob> {

    ComPtr<ID3DBlob> shaderBlob, errorBlob;
    const auto result = D3DCompileFromFile(
        filePath.wstring().c_str(),
        nullptr,
        includeHandler,
        entrypointName,
        target,
        0, 0,
        &shaderBlob,
        &errorBlob);
    if (FAILED(result)) {
        if (errorBlob) {
            std::string errorMsg(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
            throw std::runtime_error("Shader compilation error: " + errorMsg);
        } else {
            Utils::ThrowIfFailed(result);
        }
    }
    
    return shaderBlob;
}

auto BeShader::ParseHeader(const std::string& src) -> Json {
    const std::string startTag = "@be-shader-header";
    const std::string endTag   = "@be-shader-header-end";

    Json metadata = Json::object();

    const size_t startPos = src.find(startTag);
    if (startPos == std::string::npos) return metadata;
    const size_t endPos = src.find(endTag, startPos);
    if (endPos == std::string::npos) return metadata;

    // Find the start of JSON content (after the tag and newline)
    size_t jsonStart = src.find('\n', startPos);
    if (jsonStart == std::string::npos || jsonStart >= endPos) return metadata;
    jsonStart++; // Move past newline

    // Extract content between tags
    std::string jsonContent = src.substr(jsonStart, endPos - jsonStart);

    // Remove leading/trailing whitespace and newlines
    jsonContent.erase(0, jsonContent.find_first_not_of(" \t\r\n"));
    jsonContent.erase(jsonContent.find_last_not_of(" \t\r\n") + 1);

    // Parse JSON
    try {
        metadata = Json::parse(jsonContent);
    } catch (const Json::parse_error& e) {
        std::string msg = e.what();
        throw std::runtime_error("Failed to parse shader header JSON: " + std::string(msg));
    }
    
    return metadata;
}

