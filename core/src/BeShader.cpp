#include "BeShader.h"

#include <cassert>
#include <d3dcompiler.h>
#include <umbrellas/include-glm.h>

#include "BeRenderer.h"
#include "BeShaderIncludeHandler.hpp"
#include "BeShaderTools.h"
#include "Utils.h"
#include <umbrellas/include-libassert.h>

std::string BeShader::StandardShaderIncludePath = "src/shaders/";

auto BeShader::Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    be_assert(
        std::filesystem::exists(filePath), 
        "Shader file doesn't exist: " + filePath.string()
    );
    
    const auto& device = renderer.GetDevice();
    auto shader = std::make_shared<BeShader>();
    shader->Path = filePath.string();
    
    auto src = BeShaderTools::ReadFile(filePath);
    auto header = BeShaderTools::ParseFor(src, "@be-shader:");
    
    
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
        be_assert(header.contains("topology"), "", filePath);
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
            be_assert(false, "Unsupported topology", filePath);
        }
    }
    
    auto shaderErrMsgLambda = [&](const std::pair<HRESULT, ComPtr<ID3DBlob>>& err, const std::string& shaderStage) -> std::string {
        auto hrText = std::string (BeShaderTools::Trim(Utils::HResultToStr(err.first), " \n\r\t"));
        auto dxText = std::string("D3D Compiler didn't produce an error message.");
        if (err.second) {
            dxText = std::string (static_cast<const char*>(err.second->GetBufferPointer()) );
        }
        return 
        "1. Shader compilation error. \n"
        "2. Path to shader: " + filePath.string() + "\n"
        "3. Shader stage that failed: " + shaderStage + "\n"
        "4. HRESULT: " + hrText + "\n"
        "5. Compiler output: " + dxText + "\n"
        "\n"
        "Source code:\n\n" + src + "\n\n Source code end.";
    };
    
    if (header.contains("vertex")) {
        shader->ShaderType = BeShaderType::Vertex;

        std::string vertexFunctionName = header.at("vertex");
        auto result = CompileBlob(src, vertexFunctionName.c_str(), "vs_5_0", &includeHandler);
        be_assert(result, shaderErrMsgLambda(result.error(), "vertex"));
        auto blob = result.value();
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


        auto hullResult = CompileBlob(src, hullFunctionName.c_str(), "hs_5_0", &includeHandler);
        be_assert(hullResult, shaderErrMsgLambda(hullResult.error(), "hull"));
        auto domainResult = CompileBlob(src, domainFunctionName.c_str(), "ds_5_0", &includeHandler); 
        be_assert(domainResult, shaderErrMsgLambda(domainResult.error(), "domain"));
        ComPtr<ID3DBlob> hullBlob = hullResult.value();
        ComPtr<ID3DBlob> domainBlob = domainResult.value();
        Utils::Check << device->CreateHullShader(hullBlob->GetBufferPointer(), hullBlob->GetBufferSize(), nullptr, &shader->HullShader);
        Utils::Check << device->CreateDomainShader(domainBlob->GetBufferPointer(), domainBlob->GetBufferSize(), nullptr, &shader->DomainShader);
    }
    
    if (header.contains("pixel")) {
        be_assert(header.contains("targets"), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        auto result = CompileBlob(src, pixelFunctionName.c_str(), "ps_5_0", &includeHandler);
        be_assert(result, shaderErrMsgLambda(result.error(), "pixel"));
        auto blob = result.value();
        Utils::Check << device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->PixelShader);

        Json targets = header.at("targets");
        for (const auto& target : targets.items()) {
            const std::string& targetName = target.key();
            uint32_t targetSlot = target.value().get<uint32_t>();
            
            be_assert(!shader->PixelTargets.contains(targetName), "", filePath);
            be_assert(!shader->PixelTargetsInverse.contains(targetSlot), "", filePath);

            shader->PixelTargets[targetName] = targetSlot;
            shader->PixelTargetsInverse[targetSlot] = targetName;
        }
    }

    return shader;
}

auto BeShader::CompileBlob(
    const std::string& src,
    const char* entrypointName,
    const char* target,
    BeShaderIncludeHandler* includeHandler
) -> 
std::expected <
    ComPtr<ID3DBlob>, 
    std::pair<HRESULT, ComPtr<ID3DBlob>>
> 
{
    
    ComPtr<ID3DBlob> shaderBlob, errorBlob;
    const auto result = D3DCompile(
        src.c_str(),
        src.length(),
        nullptr,
        nullptr,
        includeHandler,
        entrypointName,
        target,
        0, 0,
        &shaderBlob,
        &errorBlob);
    if (FAILED(result)) {
        auto err = std::pair<HRESULT, ComPtr<ID3DBlob>>(result, errorBlob);
        return std::unexpected(err);
    }
    
    return shaderBlob;
}
