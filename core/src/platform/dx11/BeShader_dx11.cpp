#include "BeShader.h"
#include "BeShaderImpl.h"
#include "BeRenderer.h"
#include "BeRendererImpl.h"
#include "DxUtils.h"

#include <cassert>
#include <d3dcompiler.h>
#include <expected>
#include <nlohmann/json.hpp>
#include <umbrellas/include-glm.h>
#include <umbrellas/include-libassert.h>

#include "BeShaderTools.h"

BeShader::BeShader() = default;
BeShader::~BeShader() = default;

using Json = nlohmann::ordered_json;

class BeShaderIncludeHandler : public ID3DInclude {
private:
    std::filesystem::path _shaderDir;
    std::filesystem::path _globalIncludeDir;
public:
    BeShaderIncludeHandler(const std::string& shaderDir, const std::string& globalIncludeDir)
        : _shaderDir(shaderDir), _globalIncludeDir(globalIncludeDir) {}

    HRESULT STDMETHODCALLTYPE Open(
        D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName,
        LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override {
        std::filesystem::path filePath;
        const std::string fileName = pFileName;
        if (IncludeType == D3D_INCLUDE_SYSTEM)
            filePath = _globalIncludeDir / fileName;
        else if (IncludeType == D3D_INCLUDE_LOCAL)
            filePath = _shaderDir / fileName;
        if (!std::filesystem::exists(filePath)) return E_FAIL;
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) return E_FAIL;
        const auto fileSize = file.tellg();
        file.seekg(0);
        auto buffer = std::make_unique<char[]>(fileSize);
        file.read(buffer.get(), fileSize);
        if (!file) return E_FAIL;
        *ppData = buffer.release();
        *pBytes = static_cast<UINT>(fileSize);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override {
        delete[] static_cast<const char*>(pData);
        return S_OK;
    }
};

static auto CompileBlob(
    const std::string& src,
    const char* entrypointName,
    const char* target,
    BeShaderIncludeHandler* includeHandler
) -> std::expected<ComPtr<ID3DBlob>, std::pair<HRESULT, ComPtr<ID3DBlob>>> {
    ComPtr<ID3DBlob> shaderBlob, errorBlob;
    const auto result = D3DCompile(
        src.c_str(), src.length(), nullptr, nullptr,
        includeHandler, entrypointName, target, 0, 0,
        &shaderBlob, &errorBlob);
    if (FAILED(result)) {
        return std::unexpected(std::pair<HRESULT, ComPtr<ID3DBlob>>(result, errorBlob));
    }
    return shaderBlob;
}

static auto TopologyToD3D(BeTopology t) -> D3D11_PRIMITIVE_TOPOLOGY {
    switch (t) {
        case BeTopology::TriangleList:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case BeTopology::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case BeTopology::PatchList3:    return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        default:                        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

auto BeShader::Create(const std::filesystem::path& filePath, BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    be_assert(std::filesystem::exists(filePath), "Shader file doesn't exist: " + filePath.string());

    auto* rendererImpl = renderer.GetPlatformImpl();
    const auto& device = rendererImpl->device;
    auto shader = std::make_shared<BeShader>();
    shader->_impl = std::make_unique<BeShaderImpl>();

    auto src = BeShaderTools::ReadFile(filePath);
    auto [header, shaderName] = BeShaderTools::ParseFor(src, "@be-shader:");
    shader->Name = shaderName;

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
        if (topology == "triangle-list")      shader->Topology = BeTopology::TriangleList;
        else if (topology == "triangle-strip") shader->Topology = BeTopology::TriangleStrip;
        else if (topology == "patch-list-3")  shader->Topology = BeTopology::PatchList3;
        else be_assert(false, "Unsupported topology", filePath);
    }

    auto shaderErrMsgLambda = [&](const std::pair<HRESULT, ComPtr<ID3DBlob>>& err, const std::string& shaderStage) -> std::string {
        auto hrText = std::string(BeShaderTools::Trim(DxUtils::HResultToStr(err.first), " \n\r\t"));
        auto dxText = std::string("D3D Compiler didn't produce an error message.");
        if (err.second) {
            dxText = std::string(static_cast<const char*>(err.second->GetBufferPointer()));
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

        auto vertexFunctionName = std::string(header.at("vertex"));
        auto result = CompileBlob(src, vertexFunctionName.c_str(), "vs_5_0", &includeHandler);
        be_assert(result, shaderErrMsgLambda(result.error(), "vertex"));
        auto blob = result.value();
        DxUtils::Check << device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->_impl->vertexShader);

        if (header.contains("vertexLayout")) {
            auto vertexLayoutJson = header["vertexLayout"];
            auto inputLayout = std::vector<D3D11_INPUT_ELEMENT_DESC>();
            inputLayout.reserve(vertexLayoutJson.size());

            for (const std::string vertexSemanticName : vertexLayoutJson) {
                static const std::unordered_map<std::string, const char*> SemanticNames = {
                    {"position", "POSITION"}, {"normal", "NORMAL"},
                    {"color3", "COLOR"}, {"color4", "COLOR"},
                    {"uv0", "TEXCOORD"}, {"uv1", "TEXCOORD1"}, {"uv2", "TEXCOORD2"},
                };
                static const std::unordered_map<std::string, DXGI_FORMAT> ElementFormats = {
                    {"position", DXGI_FORMAT_R32G32B32_FLOAT}, {"normal", DXGI_FORMAT_R32G32B32_FLOAT},
                    {"color3", DXGI_FORMAT_R32G32B32_FLOAT}, {"color4", DXGI_FORMAT_R32G32B32A32_FLOAT},
                    {"uv0", DXGI_FORMAT_R32G32_FLOAT}, {"uv1", DXGI_FORMAT_R32G32_FLOAT}, {"uv2", DXGI_FORMAT_R32G32_FLOAT},
                };
                static const std::unordered_map<std::string, uint32_t> ElementOffsets = {
                    {"position", 0}, {"normal", 12}, {"color3", 24}, {"color4", 24},
                    {"uv0", 40}, {"uv1", 48}, {"uv2", 56},
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

            DxUtils::Check << device->CreateInputLayout(
                inputLayout.data(),
                static_cast<UINT>(inputLayout.size()),
                blob->GetBufferPointer(),
                blob->GetBufferSize(),
                &shader->_impl->computedInputLayout);
        }
    }

    if (header.contains("tesselation")) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;
        auto& tesselation = header.at("tesselation");
        auto hullFunctionName = std::string(tesselation.at("hull"));
        auto domainFunctionName = std::string(tesselation.at("domain"));

        auto hullResult = CompileBlob(src, hullFunctionName.c_str(), "hs_5_0", &includeHandler);
        be_assert(hullResult, shaderErrMsgLambda(hullResult.error(), "hull"));
        auto domainResult = CompileBlob(src, domainFunctionName.c_str(), "ds_5_0", &includeHandler);
        be_assert(domainResult, shaderErrMsgLambda(domainResult.error(), "domain"));
        auto hullBlob = hullResult.value();
        auto domainBlob = domainResult.value();
        DxUtils::Check << device->CreateHullShader(hullBlob->GetBufferPointer(), hullBlob->GetBufferSize(), nullptr, &shader->_impl->hullShader);
        DxUtils::Check << device->CreateDomainShader(domainBlob->GetBufferPointer(), domainBlob->GetBufferSize(), nullptr, &shader->_impl->domainShader);
    }

    if (header.contains("pixel")) {
        be_assert(header.contains("targets"), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;

        std::string pixelFunctionName = header.at("pixel");
        auto result = CompileBlob(src, pixelFunctionName.c_str(), "ps_5_0", &includeHandler);
        be_assert(result, shaderErrMsgLambda(result.error(), "pixel"));
        auto blob = result.value();
        DxUtils::Check << device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader->_impl->pixelShader);

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
