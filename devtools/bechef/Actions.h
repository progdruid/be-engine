#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <expected>

#include "Workspace.h"

enum class Mode { Copy, Symlink };

auto Deploy(
    const Workspace& ws,
    const std::string& app,
    const std::filesystem::path& out,
    Mode mode,
    const std::optional<std::filesystem::path>& depfile
) -> std::expected<void, std::string>;

auto Check(const Workspace& ws) -> std::expected<void, std::string>;
