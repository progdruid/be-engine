#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <optional>
#include <expected>

struct Project {
    std::string Name;
    std::vector<std::string> Depends;
    std::vector<std::string> ShaderDirs;
    std::vector<std::string> AssetDirs;
};

struct Workspace {
    std::filesystem::path Root;
    std::unordered_set<std::string> Modules;
    std::unordered_set<std::string> Apps;
    std::vector<std::string> Ignore;

    auto IsMember(const std::string& name) const -> bool {
        return Modules.contains(name) || Apps.contains(name);
    }
};

auto FindRoot(std::filesystem::path dir) -> std::optional<std::filesystem::path>;
auto LoadWorkspace(const std::filesystem::path& rootDir) -> std::expected<Workspace, std::string>;
auto LoadProject(const Workspace& ws, const std::string& name) -> std::expected<Project, std::string>;
auto ResolveProjects(const Workspace& ws, const std::string& app) -> std::expected<std::vector<Project>, std::string>;
