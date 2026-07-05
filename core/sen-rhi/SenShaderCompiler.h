#pragma once
#include <slang.h>
#include <slang-com-ptr.h>
#include <filesystem>
#include <string>
#include <vector>
#include <expected>
#include <umbrellas/common.hpp>

class SenShaderCompiler {
    hide static Slang::ComPtr<slang::IGlobalSession> _globalSession;

    expose
    static std::vector<std::filesystem::path> SearchPaths;

    static auto AddSearchPath(std::filesystem::path path) -> void;

    static void Launch();

    static auto Compile(
        const std::filesystem::path& filePath,
        const std::string& entryPoint,
        SlangStage stage,
        SlangCompileTarget target
    ) -> std::expected<Slang::ComPtr<ISlangBlob>, std::string>;
};
