#include "BeShader.h"

#include <umbrellas/include-glm.h>

#include "BeRenderer.h"
#include "BeShaderCompiler.h"
#include "BeShaderTools.h"
#include "Utils.h"
#include <umbrellas/include-libassert.h>

auto BeShader::Create(const std::filesystem::path& filePath, const BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    be_assert(
        std::filesystem::exists(filePath),
        "Shader file doesn't exist: " + filePath.string()
    );

    const auto& device = renderer.GetDevice();
    auto shader = std::make_shared<BeShader>();

    auto src = BeShaderTools::ReadFile(filePath);
    auto [header, shaderName] = BeShaderTools::ParseFor(src, "@be-shader:");
    shader->Name = shaderName;

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

    auto shaderErrMsg = [&](const std::string& err, const std::string& stage) -> std::string {
        return
            "1. Shader compilation error.\n"
            "2. Path to shader: " + filePath.string() + "\n"
            "3. Shader stage that failed: " + stage + "\n"
            "4. Compiler output:\n" + err + "\n"
            "5. Source code:\n\n" + src;
    };

    if (header.contains("vertex")) {
        shader->ShaderType = BeShaderType::Vertex;

        auto vertexFunctionName = std::string(header.at("vertex"));
        auto result = BeShaderCompiler::Compile(filePath, vertexFunctionName, SLANG_STAGE_VERTEX);
        be_assert(result, shaderErrMsg(result.error(), "vertex"));
        auto& blob = result.value();
        Utils::Check << device->CreateVertexShader(blob->getBufferPointer(), blob->getBufferSize(), nullptr, &shader->VertexShader);

        if (header.contains("vertexLayout")) {
            auto vertexLayoutJson = header["vertexLayout"];

            auto inputLayout = std::vector<D3D11_INPUT_ELEMENT_DESC>();
            inputLayout.reserve(vertexLayoutJson.size());

            for (const std::string vertexSemanticName : vertexLayoutJson) {
                static const std::unordered_map<std::string, const char*> SemanticNames = {
                    {"position", "POSITION"},
                    {"normal", "NORMAL"},
                    {"color3", "COLOR"},
                    {"color4", "COLOR"},
                    {"uv0", "TEXCOORD"},
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

                auto elementDesc = D3D11_INPUT_ELEMENT_DESC();
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
                blob->getBufferPointer(),
                blob->getBufferSize(),
                &shader->ComputedInputLayout);
        }
    }

    if (header.contains("tesselation")) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;

        auto& tesselation = header.at("tesselation");
        auto hullFunctionName = std::string(tesselation.at("hull"));
        auto domainFunctionName = std::string(tesselation.at("domain"));

        auto hullResult = BeShaderCompiler::Compile(filePath, hullFunctionName, SLANG_STAGE_HULL);
        be_assert(hullResult, shaderErrMsg(hullResult.error(), "hull"));
        auto domainResult = BeShaderCompiler::Compile(filePath, domainFunctionName, SLANG_STAGE_DOMAIN);
        be_assert(domainResult, shaderErrMsg(domainResult.error(), "domain"));

        auto& hullBlob = hullResult.value();
        auto& domainBlob = domainResult.value();
        Utils::Check << device->CreateHullShader(hullBlob->getBufferPointer(), hullBlob->getBufferSize(), nullptr, &shader->HullShader);
        Utils::Check << device->CreateDomainShader(domainBlob->getBufferPointer(), domainBlob->getBufferSize(), nullptr, &shader->DomainShader);
    }

    if (header.contains("pixel")) {
        be_assert(header.contains("targets"), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        auto result = BeShaderCompiler::Compile(filePath, pixelFunctionName, SLANG_STAGE_PIXEL);
        be_assert(result, shaderErrMsg(result.error(), "pixel"));
        auto& blob = result.value();
        Utils::Check << device->CreatePixelShader(blob->getBufferPointer(), blob->getBufferSize(), nullptr, &shader->PixelShader);

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
