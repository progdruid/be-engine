#include "BeShader.h"

#include <cassert>
#include <d3dcompiler.h>
#include <glm/glm.hpp>

#include "BeRenderer.h"
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


    if (header.contains("material")) {
        HasMaterial = true;
        const auto& materialJson = header.at("material");
        for (const auto& materialJsonItem : materialJson.items()) {
            const auto& key = materialJsonItem.key();
            const auto& value = materialJsonItem.value();

            if (value["type"].get<std::string>() == "texture2d") {
                BeMaterialTexturePropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.SlotIndex = value.at("slot").get<int>();
                descriptor.DefaultTexturePath = value.at("default").get<std::string>();
                MaterialTextureProperties.push_back(descriptor);
                continue;
            }

            static std::unordered_map<std::string, BeMaterialPropertyDescriptor::Type> typeMap = {
                {"float", BeMaterialPropertyDescriptor::Type::Float},
                {"float2", BeMaterialPropertyDescriptor::Type::Float2},
                {"float3", BeMaterialPropertyDescriptor::Type::Float3},
                {"float4", BeMaterialPropertyDescriptor::Type::Float4},
            };
            
            BeMaterialPropertyDescriptor descriptor;
            descriptor.Name = key;
            descriptor.PropertyType = typeMap.at(value.at("type").get<std::string>());

            if (descriptor.PropertyType == BeMaterialPropertyDescriptor::Type::Float) {
                descriptor.DefaultValue.push_back(value.at("default").get<float>());
            } else if (descriptor.PropertyType == BeMaterialPropertyDescriptor::Type::Float2) {
                const auto vec = value.at("default").get<std::vector<float>>();
                assert(vec.size() == 2);
                descriptor.DefaultValue = vec;
            } else if (descriptor.PropertyType == BeMaterialPropertyDescriptor::Type::Float3) {
                const auto vec = value.at("default").get<std::vector<float>>();
                assert(vec.size() == 3);
                descriptor.DefaultValue = vec;
            } else if (descriptor.PropertyType == BeMaterialPropertyDescriptor::Type::Float4) {
                const auto vec = value.at("default").get<std::vector<float>>();
                assert(vec.size() == 4);
                descriptor.DefaultValue = vec;
            }

            MaterialProperties.push_back(descriptor);
        }
    }

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
    //const std::string startTag = "@be-shader-header";
    //
    //size_t startPos = src.find(startTag);
    //if (startPos == std::string::npos) return;
    //startPos = src.find('\n', startPos) + 1;
    //
    //const size_t endPos = src.find("*/", startPos);
    //if (endPos == std::string::npos) return;
    //
    //std::string_view content = Take(src, startPos, endPos);
    //const std::vector<std::string_view> lines = Split(content, "\n")
    
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

auto BeShader::Take(const std::string_view str, const size_t start, const size_t end) -> std::string_view {
    return str.substr(start, end - start);
}

auto BeShader::Trim(const std::string_view str, const char* trimmedChars) -> std::string_view {
    return Take(str, str.find_first_not_of(trimmedChars), str.find_last_not_of(trimmedChars) + 1);
}

auto BeShader::Split(std::string_view str, const char* delimiters) -> std::vector<std::string_view> {
    std::vector<std::string_view> result;
    size_t start = 0;
    size_t delim = str.find_first_of(delimiters, start);
    while (delim != std::string_view::npos) {
        result.push_back(Take(str, start, delim));
        start = str.find_first_not_of(delimiters, delim);
        if (start == std::string_view::npos) {
            return result;
        }
        delim = str.find_first_of(delimiters, start);
    }
    result.push_back(Take(str, start, str.size()));
    return result;
}

