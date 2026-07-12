#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <expected>

#include "Workspace.h"

struct ShaderGenOptions {
    std::optional<std::string> Project;
    std::optional<std::filesystem::path> File;
    bool CheckOnly = false;
    bool Watch = false;
};

auto ShaderGen(const Workspace& ws, const ShaderGenOptions& options) -> std::expected<void, std::string>;
