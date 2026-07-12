#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <expected>

#include "Workspace.h"

enum class Mode { Copy, Symlink };

struct DeployEntry {
    std::filesystem::path Src;
    std::filesystem::path DstRel;
};

struct DeployFiles {
    std::vector<DeployEntry> Entries;
    std::vector<std::filesystem::path> Inputs;
    std::vector<std::filesystem::path> SourceDirs;
};

auto CollectAssets(const Workspace& ws, const std::vector<Project>& projects) -> std::expected<DeployFiles, std::string>;
auto CollectShaders(const Workspace& ws, const std::vector<Project>& projects) -> std::expected<DeployFiles, std::string>;

auto Deploy(
    const Workspace& ws,
    const std::string& app,
    const std::filesystem::path& out,
    Mode mode,
    const std::optional<std::filesystem::path>& depfile
) -> std::expected<void, std::string>;
