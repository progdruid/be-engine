#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <expected>

struct ShaderGenOptions {
    std::optional<std::string> Project;
    std::optional<std::filesystem::path> File;
    bool CheckOnly = false;
    bool Watch = false;
};

auto ShaderGen(const ShaderGenOptions& options) -> std::expected<void, std::string>;
