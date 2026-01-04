#include "BeShader.h"

#include <cassert>
#include <d3dcompiler.h>
#include <umbrellas/include-glm.h>

#include "BeRenderer.h"
#include "BeShaderIncludeHandler.hpp"
#include "BeShaderTools.h"
#include "Utils.h"

std::string BeShader::StandardShaderIncludePath = "src/shaders/";

auto BeShader::Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    assert(std::filesystem::exists(filePath));
    const auto& device = renderer.GetDevice();
    auto shader = std::make_shared<BeShader>();
    shader->Path = filePath.string();
    
    const auto header = BeShaderTools::ParseShaderMetadata(filePath);
    
    BeShaderIncludeHandler includeHandler(
        filePath.parent_path().string(),
        StandardShaderIncludePath
    );
    
    if (header.contains("materials")) {
        shader->HasMaterial = true;
        const auto& materialLinksJson = header.at("materials");

        for (const auto& materialLinkJson : materialLinksJson.items()) {

            auto linkName = std::string(materialLinkJson.key());
            auto schemeName = std::string(materialLinkJson.value()["scheme"]);
            auto schemeSlot = uint8_t(materialLinkJson.value()["slot"]);

            shader->_materialSchemeNames[linkName] = schemeName;
            shader->_materialSlots[linkName] = schemeSlot;
            shader->_materialSlotsByScheme[schemeName] = schemeSlot;
        }
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
        ComPtr<ID3DBlob> blob = CompileBlob(filePath, vertexFunctionName.c_str(), "vs_5_0", &includeHandler);
        Utils::Check << device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->VertexShader);

        //input layout
        if (header.contains("vertexLayout")) {
            Json vertexLayoutJson = header["vertexLayout"];

            std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
            inputLayout.reserve(vertexLayoutJson.size());

            for (const std::string vertexSemanticName : vertexLayoutJson) {
                static const std::unordered_map<std::string, const char*> SemanticNames = {
                    {"position", "POSITION"},
                    {"normal", "NORMAL"},
                    {"color3", "COLOR"},
                    {"color4", "COLOR"},
                    {"uv0", "TEXCOORD"}, //????????
                    {"uv1", "TEXCOORD1"},
                    {"uv2", "TEXCOORD2"},
                };
                static const std::unordered_map<std::string, DXGI_FORMAT> ElementFormats = {
                    {"position", DXGI_FORMAT_R32G32B32_FLOAT},
                    {"normal", DXGI_FORMAT_R32G32B32_FLOAT},
                    {"color3", DXGI_FORMAT_R32G32B32_FLOAT},
                    {"color4", DXGI_FORMAT_R32G32B32A32_FLOAT},
                    {"uv0", DXGI_FORMAT_R32G32_FLOAT},
                    {"uv1", DXGI_FORMAT_R32G32_FLOAT},
                    {"uv2", DXGI_FORMAT_R32G32_FLOAT},
                };
                static const std::unordered_map<std::string, uint32_t> ElementOffsets = {
                    {"position", 0},
                    {"normal",  12},
                    {"color3",  24},
                    {"color4",  24},
                    {"uv0",     40},
                    {"uv1",     48},
                    {"uv2",     56},
                };

                D3D11_INPUT_ELEMENT_DESC elementDesc;
                elementDesc.SemanticIndex = 0;
                elementDesc.InputSlot = 0;
                elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                elementDesc.InstanceDataStepRate = 0;
                elementDesc.AlignedByteOffset = ElementOffsets.at(vertexSemanticName);
                elementDesc.SemanticName = SemanticNames.at(vertexSemanticName);
                elementDesc.Format = ElementFormats.at(vertexSemanticName);
                
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
        ComPtr<ID3DBlob> hullBlob = CompileBlob(filePath, hullFunctionName.c_str(), "hs_5_0", &includeHandler);
        ComPtr<ID3DBlob> domainBlob = CompileBlob(filePath, domainFunctionName.c_str(), "ds_5_0", &includeHandler);
        Utils::Check << device->CreateHullShader(hullBlob->GetBufferPointer(), hullBlob->GetBufferSize(), nullptr, &shader->HullShader);
        Utils::Check << device->CreateDomainShader(domainBlob->GetBufferPointer(), domainBlob->GetBufferSize(), nullptr, &shader->DomainShader);
    }
    
    if (header.contains("pixel")) {
        assert(header.contains("targets"));
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        ComPtr<ID3DBlob> blob = CompileBlob(filePath, pixelFunctionName.c_str(), "ps_5_0", &includeHandler);
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
