#pragma once

#include <string>
#include <optional>

#include "Workspace.h"

auto GenerateShaderSource(const ShaderFile& shader) -> std::optional<std::string>;
