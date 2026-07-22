#pragma once

#include <string>
#include <filesystem>
#include <expected>

enum class Mode { Copy, Symlink };

auto Cook(
    const std::string& app,
    const std::filesystem::path& out,
    Mode mode
) -> std::expected<void, std::string>;
