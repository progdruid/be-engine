#include "BeShader.h"

#include "BeRenderer.h"
#include "BeShaderTools.h"
#include "Utils.h"
#include <umbrellas/include-libassert.h>

#include "sen-rhi/SenShaderCompiler.h"
#include <sen-rhi/dx11/SenDx11Backend.h>

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
            shader->Topology = SenTopology::TriangleList;
        }
        else if (topology == "triangle-strip") {
            shader->Topology = SenTopology::TriangleStrip;
        }
        else if (topology == "patch-list-3") {
            shader->Topology = SenTopology::PatchList3;
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
        auto result = SenShaderCompiler::Compile(filePath, vertexFunctionName, SLANG_STAGE_VERTEX);
        be_assert(result, shaderErrMsg(result.error(), "vertex"));
        auto& blob = result.value();

        shader->ShaderVertex = SenDx11Backend::Get().CreateShader({
            .Blob = blob->getBufferPointer(),
            .BlobSize = static_cast<uint32_t>(blob->getBufferSize()),
            .Stage = SenShaderStage::Vertex,
        });

        if (header.contains("vertexLayout")) {
            auto vertexLayoutJson = header["vertexLayout"];

            static const std::unordered_map<std::string, SenFormat> ElementFormats = {
                {"position", SenFormat::RGB32_Float},
                {"normal",   SenFormat::RGB32_Float},
                {"color3",   SenFormat::RGB32_Float},
                {"color4",   SenFormat::RGBA32_Float},
                {"uv0",      SenFormat::RG32_Float},
                {"uv1",      SenFormat::RG32_Float},
                {"uv2",      SenFormat::RG32_Float},
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

            std::vector<SenVertexLayoutDesc::Element> layoutElements;
            for (const std::string& semantic : vertexLayoutJson) {
                layoutElements.push_back({
                    .Semantic = semantic,
                    .Format = ElementFormats.at(semantic),
                    .Offset = ElementOffsets.at(semantic),
                });
            }

            shader->VertexLayout = SenDx11Backend::Get().CreateVertexLayout({
                .Elements = layoutElements,
                .VertexShaderBytecode = blob->getBufferPointer(),
                .VertexShaderBytecodeSize = static_cast<uint32_t>(blob->getBufferSize()),
            });
        }
    }

    if (header.contains("tesselation")) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;

        auto& tesselation = header.at("tesselation");
        auto hullFunctionName = std::string(tesselation.at("hull"));
        auto domainFunctionName = std::string(tesselation.at("domain"));

        auto hullResult = SenShaderCompiler::Compile(filePath, hullFunctionName, SLANG_STAGE_HULL);
        be_assert(hullResult, shaderErrMsg(hullResult.error(), "hull"));
        auto domainResult = SenShaderCompiler::Compile(filePath, domainFunctionName, SLANG_STAGE_DOMAIN);
        be_assert(domainResult, shaderErrMsg(domainResult.error(), "domain"));

        auto& hullBlob = hullResult.value();
        auto& domainBlob = domainResult.value();

        shader->ShaderHull = SenDx11Backend::Get().CreateShader({
            .Blob = hullBlob->getBufferPointer(),
            .BlobSize = static_cast<uint32_t>(hullBlob->getBufferSize()),
            .Stage = SenShaderStage::Hull,
        });
        shader->ShaderDomain = SenDx11Backend::Get().CreateShader({
            .Blob = domainBlob->getBufferPointer(),
            .BlobSize = static_cast<uint32_t>(domainBlob->getBufferSize()),
            .Stage = SenShaderStage::Domain,
        });
    }

    if (header.contains("pixel")) {
        be_assert(header.contains("targets"), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        auto result = SenShaderCompiler::Compile(filePath, pixelFunctionName, SLANG_STAGE_PIXEL);
        be_assert(result, shaderErrMsg(result.error(), "pixel"));
        auto& blob = result.value();

        shader->ShaderPixel = SenDx11Backend::Get().CreateShader({
            .Blob = blob->getBufferPointer(),
            .BlobSize = static_cast<uint32_t>(blob->getBufferSize()),
            .Stage = SenShaderStage::Pixel,
        });

        Json targets = header.at("targets");
        for (const auto& target : targets.items()) {
            const std::string& targetName = target.key();
            uint32_t targetSlot = target.value()["slot"].get<uint32_t>();

            be_assert(!shader->PixelTargets.contains(targetName), "", filePath);
            be_assert(!shader->PixelTargetsInverse.contains(targetSlot), "", filePath);

            shader->PixelTargets[targetName] = targetSlot;
            shader->PixelTargetsInverse[targetSlot] = targetName;
        }
    }

    return shader;
}
