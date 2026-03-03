#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include <filesystem>
#include <string>
#include <expected>
#include <umbrellas/access-modifiers.hpp>

class BeShaderCompiler {
    hide static Slang::ComPtr<slang::IGlobalSession> _globalSession;

    expose
    static std::string AssetShadersPath;
    static std::string StandardShadersPath;

    static void Launch();

    static auto Compile(
        const std::filesystem::path& filePath,
        const std::string& entryPoint,
        SlangStage stage
    ) -> std::expected<Slang::ComPtr<ISlangBlob>, std::string>;
};
