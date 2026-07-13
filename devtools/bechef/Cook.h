#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <expected>

enum class Mode { Copy, Symlink };

auto Cook(
    const std::string& app,
    const std::filesystem::path& out,
    Mode mode,
    const std::optional<std::filesystem::path>& depfile
) -> std::expected<void, std::string>;
