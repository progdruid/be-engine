#include "SenShaderCompiler.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <cstring>
#include <fstream>
#include <umbrellas/include-libassert.h>

namespace {
    Slang::ComPtr<slang::IGlobalSession> globalSession;

    auto ToSlangStage(SenShaderStage stage) -> SlangStage {
        switch (stage) {
            case SenShaderStage::Vertex:  return SLANG_STAGE_VERTEX;
            case SenShaderStage::Hull:    return SLANG_STAGE_HULL;
            case SenShaderStage::Domain:  return SLANG_STAGE_DOMAIN;
            case SenShaderStage::Pixel:   return SLANG_STAGE_PIXEL;
            case SenShaderStage::Compute: return SLANG_STAGE_COMPUTE;
        }
        be_assert(false, "SenShaderCompiler: unsupported shader stage");
        return SLANG_STAGE_NONE;
    }
}

std::vector<std::filesystem::path> SenShaderCompiler::SearchPaths;

auto SenShaderCompiler::AddSearchPath(std::filesystem::path path) -> void {
    SearchPaths.push_back(std::move(path));
}

auto SenShaderCompiler::Launch() -> void {
    SlangResult result = slang::createGlobalSession(globalSession.writeRef());
    be_assert(SLANG_SUCCEEDED(result), "Failed to create Slang global session");
}

auto SenShaderCompiler::Compile(
    const std::filesystem::path& filePath,
    const std::string& entryPoint,
    SenShaderStage stage
) -> std::expected<CompileResult, std::string> {

    be_assert(globalSession, "SenShaderCompiler was never initialized. Make sure to call Init.");

    const SlangStage slangStage = ToSlangStage(stage);

    auto extractDiag = [](ISlangBlob* diag) -> std::string {
        return diag
        ? std::string(static_cast<const char*>(diag->getBufferPointer()), diag->getBufferSize())
        : std::string("(no diagnostics)");
    };

    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("glsl_450");

    auto searchPathStrings = std::vector<std::string>();
    searchPathStrings.reserve(SearchPaths.size());
    for (const auto& path : SearchPaths) {
        searchPathStrings.push_back(path.string());
    }
    auto searchPaths = std::vector<const char*>();
    searchPaths.reserve(searchPathStrings.size());
    for (const auto& str : searchPathStrings) {
        searchPaths.push_back(str.c_str());
    }

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;
    sessionDesc.searchPaths = searchPaths.data();
    sessionDesc.searchPathCount = static_cast<SlangInt>(searchPaths.size());

    Slang::ComPtr<slang::ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    // loadModule only resolves .slang files, so we read the source ourselves
    auto file = std::ifstream(filePath);
    auto src = std::string(std::istreambuf_iterator<char>(file), {});

    auto moduleName = filePath.stem().string();
    auto pathStr = filePath.string();
    Slang::ComPtr<ISlangBlob> loadDiag;
    slang::IModule* module = session->loadModuleFromSourceString(
        moduleName.c_str(), pathStr.c_str(), src.c_str(), loadDiag.writeRef()
    );
    if (!module) {
        return std::unexpected(
            "Failed to load shader '" + filePath.string() + "':\n" + extractDiag(loadDiag.get())
        );
    }

    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    Slang::ComPtr<ISlangBlob> epDiag;
    SlangResult epResult = module->findAndCheckEntryPoint(
        entryPoint.c_str(), slangStage, entryPointObj.writeRef(), epDiag.writeRef()
    );
    if (SLANG_FAILED(epResult)) {
        return std::unexpected(
            "Entry point '" + entryPoint + "' error:\n" + extractDiag(epDiag.get())
        );
    }

    slang::IComponentType* components[] = { module, entryPointObj.get() };
    Slang::ComPtr<slang::IComponentType> composed;
    Slang::ComPtr<ISlangBlob> composeDiag;
    session->createCompositeComponentType(components, 2, composed.writeRef(), composeDiag.writeRef());

    Slang::ComPtr<slang::IComponentType> linked;
    Slang::ComPtr<ISlangBlob> linkDiag;
    SlangResult linkResult = composed->link(linked.writeRef(), linkDiag.writeRef());
    if (SLANG_FAILED(linkResult)) {
        return std::unexpected("Link error:\n" + extractDiag(linkDiag.get()));
    }

    Slang::ComPtr<ISlangBlob> code;
    Slang::ComPtr<ISlangBlob> codeDiag;
    SlangResult codeResult = linked->getEntryPointCode(0, 0, code.writeRef(), codeDiag.writeRef());
    if (SLANG_FAILED(codeResult) || !code) {
        return std::unexpected("Code gen error:\n" + extractDiag(codeDiag.get()));
    }

    const size_t byteSize = code->getBufferSize();
    be_assert(byteSize % sizeof(uint32_t) == 0, "SenShaderCompiler: SPIR-V blob is not word-aligned");

    auto result = CompileResult();
    result.Bytecode.resize(byteSize / sizeof(uint32_t));
    std::memcpy(result.Bytecode.data(), code->getBufferPointer(), byteSize);

    const auto sourcePath = std::filesystem::weakly_canonical(filePath);
    const SlangInt32 dependencyCount = module->getDependencyFileCount();
    for (SlangInt32 i = 0; i < dependencyCount; ++i) {
        auto dependency = std::filesystem::weakly_canonical(module->getDependencyFilePath(i));
        if (dependency != sourcePath) {
            result.Includes.push_back(std::move(dependency));
        }
    }

    return result;
}
