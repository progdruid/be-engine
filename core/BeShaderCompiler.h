#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <expected>
#include <umbrellas/common.hpp>

enum class BeShaderStage : uint8_t {
    Vertex,
    Hull,
    Domain,
    Pixel,
    Compute,
};

class BeShaderCompiler {
    expose
    struct CompileResult {
        std::vector<uint32_t> Bytecode;
        std::vector<std::filesystem::path> Includes;
    };

    static std::vector<std::filesystem::path> SearchPaths;

    static auto AddSearchPath(std::filesystem::path path) -> void;

    static void Launch();

    static auto Compile(
        const std::filesystem::path& filePath,
        const std::string& entryPoint,
        BeShaderStage stage
    ) -> std::expected<CompileResult, std::string>;
};
