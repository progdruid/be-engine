#include "BeShader.h"
#include "BeShaderImpl.h"
#include "BeRenderer.h"
#include "BeRendererImpl.h"
#include "MetalUtils.h"

#import <Metal/Metal.h>

#include <cassert>
#include <fstream>
#include <umbrellas/include-glm.h>
#include <umbrellas/include-libassert.h>

#include "BeShaderTools.h"

static auto LoadMSLSource(const std::filesystem::path& mslPath, id<MTLDevice> device) -> id<MTLLibrary> {
    std::ifstream file(mslPath);
    be_assert(file.is_open(), "Failed to open MSL file: " + mslPath.string());
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    NSError* error = nil;
    NSString* nsSource = [NSString stringWithUTF8String:source.c_str()];
    id<MTLLibrary> library = [device newLibraryWithSource:nsSource options:nil error:&error];
    const bool isLibraryLoaded = library != nil;
    be_assert(isLibraryLoaded, "Metal shader compilation failed: " +
        std::string([[error localizedDescription] UTF8String]));
    return library;
}

static auto LoadMetallib(const std::filesystem::path& metallibPath, id<MTLDevice> device) -> id<MTLLibrary> {
    NSError* error = nil;
    NSString* path = [NSString stringWithUTF8String:metallibPath.c_str()];
    id<MTLLibrary> library = [device newLibraryWithFile:path error:&error];
    const bool isLibraryLoaded = library != nil;
    be_assert(isLibraryLoaded, "Failed to load .metallib: " + metallibPath.string());
    return library;
}

static auto FindShaderFile(const std::filesystem::path& hlslPath, const std::string& functionName, const std::string& stage) -> std::filesystem::path {
    auto stem = hlslPath.stem().string();
    (void)functionName;

    std::vector<std::filesystem::path> dirsToProbe;
    const auto cwd = std::filesystem::current_path();
    dirsToProbe.push_back(cwd / "bin/AARCH64/Debug/assets/shaders");
    dirsToProbe.push_back(cwd / "bin/AARCH64/Release/assets/shaders");
    dirsToProbe.push_back(hlslPath.parent_path());

    for (const auto& dir : dirsToProbe) {
        auto metallibPath = dir / (stem + "." + stage + ".metallib");
        if (std::filesystem::exists(metallibPath)) return metallibPath;

        auto metalPath = dir / (stem + "." + stage + ".metal");
        if (std::filesystem::exists(metalPath)) return metalPath;

        auto singleMetal = dir / (stem + ".metal");
        if (std::filesystem::exists(singleMetal)) return singleMetal;
    }

    return {};
}

static auto LoadFunction(
    id<MTLDevice> device,
    const std::filesystem::path& hlslPath,
    const std::string& functionName,
    const std::string& stage
) -> std::pair<id<MTLLibrary>, id<MTLFunction>> {
    auto shaderPath = FindShaderFile(hlslPath, functionName, stage);
    be_assert(!shaderPath.empty(), "No Metal shader found for " + stage + " stage of " + hlslPath.string());

    id<MTLLibrary> library;
    if (shaderPath.extension() == ".metallib") {
        library = LoadMetallib(shaderPath, device);
    } else {
        library = LoadMSLSource(shaderPath, device);
    }

    NSString* fnName = [NSString stringWithUTF8String:functionName.c_str()];
    id<MTLFunction> function = [library newFunctionWithName:fnName];
    const bool isFunctionLoaded = function != nil;
    be_assert(isFunctionLoaded, "Function '" + functionName + "' not found in " + shaderPath.string());
    return { library, function };
}

BeShader::BeShader() = default;
BeShader::~BeShader() = default;

auto BeShader::Create(const std::filesystem::path& filePath, BeRenderer& renderer) -> std::shared_ptr<BeShader> {
    be_assert(std::filesystem::exists(filePath), "Shader file doesn't exist: " + filePath.string());

    auto* rendererImpl = renderer.GetPlatformImpl();
    auto device = rendererImpl->device;
    auto shader = std::make_shared<BeShader>();
    shader->_impl = std::make_unique<BeShaderImpl>();

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
        if (topology == "triangle-list")       shader->Topology = BeTopology::TriangleList;
        else if (topology == "triangle-strip") shader->Topology = BeTopology::TriangleStrip;
        else if (topology == "patch-list-3")   shader->Topology = BeTopology::PatchList3;
        else be_assert(false, "Unsupported topology", filePath);
    }

    if (header.contains("vertex")) {
        shader->ShaderType = BeShaderType::Vertex;
        auto vertexFunctionName = std::string(header.at("vertex"));
        auto [lib, fn] = LoadFunction(device, filePath, vertexFunctionName, "vs");
        shader->_impl->vertexLibrary = lib;
        shader->_impl->vertexFunction = fn;

        if (header.contains("vertexLayout")) {
            MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
            auto vertexLayoutJson = header["vertexLayout"];
            uint32_t attrIndex = 0;

            static const std::unordered_map<std::string, uint32_t> ElementOffsets = {
                {"position", 0}, {"normal", 12}, {"color3", 24}, {"color4", 24},
                {"uv0", 40}, {"uv1", 48}, {"uv2", 56},
            };
            static const std::unordered_map<std::string, MTLVertexFormat> ElementFormats = {
                {"position", MTLVertexFormatFloat3}, {"normal", MTLVertexFormatFloat3},
                {"color3", MTLVertexFormatFloat3}, {"color4", MTLVertexFormatFloat4},
                {"uv0", MTLVertexFormatFloat2}, {"uv1", MTLVertexFormatFloat2}, {"uv2", MTLVertexFormatFloat2},
            };

            for (const std::string vertexSemanticName : vertexLayoutJson) {
                vertexDesc.attributes[attrIndex].format = ElementFormats.at(vertexSemanticName);
                vertexDesc.attributes[attrIndex].offset = ElementOffsets.at(vertexSemanticName);
                vertexDesc.attributes[attrIndex].bufferIndex = 30;
                attrIndex++;
            }

            vertexDesc.layouts[30].stride = sizeof(float) * 16;
            vertexDesc.layouts[30].stepFunction = MTLVertexStepFunctionPerVertex;

            shader->_impl->vertexDescriptor = vertexDesc;
        }
    }

    if (header.contains("tesselation")) {
        shader->ShaderType = shader->ShaderType | BeShaderType::Tesselation;
        auto& tesselation = header.at("tesselation");
        auto hullFunctionName = std::string(tesselation.at("hull"));
        auto domainFunctionName = std::string(tesselation.at("domain"));

        auto [hLib, hFn] = LoadFunction(device, filePath, hullFunctionName, "hs");
        shader->_impl->hullLibrary = hLib;
        shader->_impl->hullFunction = hFn;
        auto [dLib, dFn] = LoadFunction(device, filePath, domainFunctionName, "ds");
        shader->_impl->domainLibrary = dLib;
        shader->_impl->domainFunction = dFn;
    }

    if (header.contains("pixel")) {
        be_assert(header.contains("targets"), "", filePath);
        shader->ShaderType = shader->ShaderType | BeShaderType::Pixel;
        auto pixelFunctionName = std::string(header.at("pixel"));
        auto [lib, fn] = LoadFunction(device, filePath, pixelFunctionName, "ps");
        shader->_impl->pixelLibrary = lib;
        shader->_impl->pixelFunction = fn;

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
