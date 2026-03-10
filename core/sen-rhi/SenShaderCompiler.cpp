#include "SenShaderCompiler.h"

#include <fstream>
#include <umbrellas/include-libassert.h>

Slang::ComPtr<slang::IGlobalSession> SenShaderCompiler::_globalSession;
std::string SenShaderCompiler::AssetShadersPath = "assets/shaders/";
std::string SenShaderCompiler::StandardShadersPath = "shaders/";

auto SenShaderCompiler::Launch() -> void {
    SlangResult result = slang::createGlobalSession(_globalSession.writeRef());
    be_assert(SLANG_SUCCEEDED(result), "Failed to create Slang global session");
}

auto SenShaderCompiler::Compile(
    const std::filesystem::path& filePath,
    const std::string& entryPoint,
    SlangStage stage,
    SlangCompileTarget target
) -> std::expected<Slang::ComPtr<ISlangBlob>, std::string> {

    auto extractDiag = [](ISlangBlob* diag) -> std::string {
        return diag
        ? std::string(static_cast<const char*>(diag->getBufferPointer()), diag->getBufferSize())
        : std::string("(no diagnostics)");
    };
    
    slang::TargetDesc targetDesc = {};
    targetDesc.format = target;
    if (target == SLANG_SPIRV) {
        targetDesc.profile = _globalSession->findProfile("glsl_450");
    } else {
        targetDesc.profile = _globalSession->findProfile("sm_5_0");
    }

    const char* searchPaths[] = { AssetShadersPath.c_str(), StandardShadersPath.c_str() };

    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;
    sessionDesc.searchPaths = searchPaths;
    sessionDesc.searchPathCount = 2;

    Slang::ComPtr<slang::ISession> session;
    _globalSession->createSession(sessionDesc, session.writeRef());

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
        entryPoint.c_str(), stage, entryPointObj.writeRef(), epDiag.writeRef()
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

    return code;
}
