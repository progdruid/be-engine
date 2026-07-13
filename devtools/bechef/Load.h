#pragma once

#include <filesystem>
#include <optional>
#include <expected>
#include <string>

auto FindRoot(std::filesystem::path dir) -> std::optional<std::filesystem::path>;
auto LoadWorkspace(const std::filesystem::path& rootDir) -> std::expected<void, std::string>;
auto ReloadWorkspace() -> std::expected<void, std::string>;
