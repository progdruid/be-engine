#pragma once

#include <string>
#include <expected>

#include "ShaderCatalog.h"

struct ShaderGenerationResult {
    bool Skipped = false;
    std::string NewSource;
};

auto GenerateShaderSource(
    const ShaderFile& file,
    const SchemeScope& scope
) -> std::expected<ShaderGenerationResult, std::string>;

auto ValidateShaderFile(
    const ShaderFile& file,
    const SchemeScope& scope
) -> std::expected<void, std::string>;
