#include "BeShader.h"

#include <cassert>
#include <d3dcompiler.h>
#include <umbrellas/include-glm.h>

#include "BeRenderer.h"
#include "BeShaderIncludeHandler.hpp"
#include "Utils.h"

auto BeShader::Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    const auto& device = renderer.GetDevice();
    
    auto shader = std::make_shared<BeShader>();

    BeShaderIncludeHandler includeHandler(
        filePath.parent_path().string(),
        StandardShaderIncludePath
    );

    std::filesystem::path path = filePath;
    path += ".hlsl";
    assert(std::filesystem::exists(path));

    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string src = buffer.str();
    const Json header = ParseFor(src, "@be-shader:");
    
    
    if (header.contains("material")) {
        shader->HasMaterial = true;
        const std::string& materialName = header.at("material");
        const Json materialJson = ParseFor(src, "@be-material: " + materialName);
        BeMaterialDescriptor materialDescriptor;
        materialDescriptor.TypeName = materialName;
        
        for (const auto& materialJsonItem : materialJson.items()) {
            
            // splitting
            const auto& key = materialJsonItem.key();
            const std::string valueString = materialJsonItem.value();
            const auto parts = Split(valueString, "=");
            assert(!parts.empty());
            const auto typeParts = Split(Trim(parts[0], " \n\r\t"), "()");
            assert(!typeParts.empty());
            
            // final strings
            const std::string typeString (typeParts[0]);
            uint8_t slotIndex = 0;
            std::string defaultString;  
            
            if (typeParts.size() > 1) {
                slotIndex = std::stoi(std::string(typeParts[1]));
            }
            if (parts.size() > 1) {
                defaultString = Trim(parts[1], " \n\r\t");
            }
            
            // extracting
            if (typeString == "texture2d") {
                BeMaterialTextureDescriptor descriptor;
                descriptor.Name = key;
                descriptor.SlotIndex = slotIndex;
                descriptor.DefaultTexturePath = defaultString;
                materialDescriptor.Textures.push_back(descriptor);
            }
            else if (typeString == "sampler") {
                BeMaterialSamplerDescriptor descriptor;
                descriptor.Name = key;
                descriptor.SlotIndex = slotIndex;
                materialDescriptor.Samplers.push_back(descriptor);
            }
            else if (typeString == "float") {
                BeMaterialPropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float;
                descriptor.DefaultValue.push_back(std::stof(defaultString));
                materialDescriptor.Properties.push_back(descriptor);
            }
            else if (typeString == "float2") {
                Json j = Json::parse(defaultString, nullptr, true, true, true);
                const auto vec = j.get<std::vector<float>>();
                assert(vec.size() == 2);
                
                BeMaterialPropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float2;
                descriptor.DefaultValue = vec;
                materialDescriptor.Properties.push_back(descriptor);
            }
            else if (typeString == "float3") {
                Json j = Json::parse(defaultString, nullptr, true, true, true);
                const auto vec = j.get<std::vector<float>>();
                assert(vec.size() == 3);
                
                BeMaterialPropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float3;
                descriptor.DefaultValue = vec;
                materialDescriptor.Properties.push_back(descriptor);
            }
            else if (typeString == "float4") {
                Json j = Json::parse(defaultString, nullptr, true, true, true);
                const auto vec = j.get<std::vector<float>>();
                assert(vec.size() == 4);
                
                BeMaterialPropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Float4;
                descriptor.DefaultValue = vec;
                materialDescriptor.Properties.push_back(descriptor);
            }
            else if (typeString == "matrix") {
                std::vector<float> mat = {
                    1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    0, 0, 0, 1
                };
                
                BeMaterialPropertyDescriptor descriptor;
                descriptor.Name = key;
                descriptor.PropertyType = BeMaterialPropertyDescriptor::Type::Matrix;
                descriptor.DefaultValue = mat;
                materialDescriptor.Properties.push_back(descriptor);
            }
        }
        
        shader->MaterialDescriptors.push_back(materialDescriptor);
    }
    
    {
        assert(header.contains("topology"));
        const auto& topology = header.at("topology");

        if (topology == "triangle-list") {
            shader->Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
        else if (topology == "triangle-strip") {
            shader->Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        }
        else if (topology == "patch-list-3") {
            shader->Topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        }
        else {
            assert(false && "Unsupported topology");
        }
    }
    
    if (header.contains("vertex")) {
        shader->ShaderType = BeShaderType::Vertex;

        std::string vertexFunctionName = header.at("vertex");
        ComPtr<ID3DBlob> blob = CompileBlob(path, vertexFunctionName.c_str(), "vs_5_0", &includeHandler);
        Utils::Check << device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->VertexShader);

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

            Utils::Check << renderer.GetDevice()->CreateInputLayout(
                inputLayout.data(),
                static_cast<UINT>(inputLayout.size()),
                blob->GetBufferPointer(),
                blob->GetBufferSize(),
                &shader->ComputedInputLayout);
        }
    }

    if (header.contains("tesselation")) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;

        const Json& tesselation = header.at("tesselation");
        std::string hullFunctionName = tesselation.at("hull");
        std::string domainFunctionName = tesselation.at("domain");
        ComPtr<ID3DBlob> hullBlob = CompileBlob(path, hullFunctionName.c_str(), "hs_5_0", &includeHandler);
        ComPtr<ID3DBlob> domainBlob = CompileBlob(path, domainFunctionName.c_str(), "ds_5_0", &includeHandler);
        Utils::Check << device->CreateHullShader(hullBlob->GetBufferPointer(), hullBlob->GetBufferSize(), nullptr, &shader->HullShader);
        Utils::Check << device->CreateDomainShader(domainBlob->GetBufferPointer(), domainBlob->GetBufferSize(), nullptr, &shader->DomainShader);
    }
    
    if (header.contains("pixel")) {
        assert(header.contains("targets"));
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        ComPtr<ID3DBlob> blob = CompileBlob(path, pixelFunctionName.c_str(), "ps_5_0", &includeHandler);
        Utils::Check << device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->PixelShader);

        Json targets = header.at("targets");
        for (const auto& target : targets.items()) {
            const std::string& targetName = target.key();
            uint32_t targetSlot = target.value().get<uint32_t>();
            
            assert(!shader->PixelTargets.contains(targetName));
            assert(!shader->PixelTargetsInverse.contains(targetSlot));

            shader->PixelTargets[targetName] = targetSlot;
            shader->PixelTargetsInverse[targetSlot] = targetName;
        }
    }

    return shader;
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

auto BeShader::ParseFor(const std::string& src, const std::string& target) -> Json {
    const std::string& startTag = target;
    const std::string& endTag   = "@be-end";
    
    Json metadata = Json::object();
    
    const size_t startPos = src.find(startTag);
    if (startPos == std::string::npos) return metadata;
    const size_t endPos = src.find(endTag, startPos);
    if (endPos == std::string::npos) return metadata;
    
    size_t jsonStart = src.find('\n', startPos);
    if (jsonStart == std::string::npos || jsonStart >= endPos) return metadata;
    jsonStart++; // Move past newline
    
    std::string jsonContent = src.substr(jsonStart, endPos - jsonStart);
    
    jsonContent.erase(0, jsonContent.find_first_not_of(" \t\r\n"));
    jsonContent.erase(jsonContent.find_last_not_of(" \t\r\n") + 1);
    
    try {
        metadata = Json::parse(jsonContent, nullptr, true, true, true);
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