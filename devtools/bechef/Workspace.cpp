#include "Workspace.h"

#include <format>

auto Workspace::Get() -> Workspace& {
    static auto workspace = Workspace();
    return workspace;
}

auto Workspace::AllProjects() const -> std::vector<const Project*> {
    auto projects = std::vector<const Project*>();
    for (const auto& [name, project] : Projects) {
        if (project) projects.push_back(&*project);
    }
    return projects;
}

auto Workspace::AppProjects() const -> std::vector<const Project*> {
    auto apps = std::vector<const Project*>();
    for (const auto& [name, project] : Projects) {
        if (project && Config.Apps.contains(name)) apps.push_back(&*project);
    }
    return apps;
}

auto Workspace::FindProject(const std::string& name) const -> const ProjectOrError* {
    const auto it = Projects.find(name);
    return it == Projects.end() ? nullptr : &it->second;
}

auto Workspace::FindOwningProject(const std::filesystem::path& file) const -> std::expected<const Project*, std::string> {
    auto ec = std::error_code();
    const auto absolute = std::filesystem::weakly_canonical(std::filesystem::absolute(file), ec);
    const auto root = std::filesystem::weakly_canonical(Config.Root, ec);

    const auto relativePath = absolute.lexically_relative(root);
    if (relativePath.empty() || *relativePath.begin() == "..") {
        return std::unexpected(std::format("'{}' is outside the workspace at {}", file.string(), root.string()));
    }

    const auto name = relativePath.begin()->string();
    const auto* entry = FindProject(name);
    if (!entry) {
        return std::unexpected(std::format("'{}' belongs to '{}', which is not a project in workspace.bechef", file.string(), name));
    }
    if (!*entry) {
        return std::unexpected(entry->error());
    }
    return &**entry;
}

auto Workspace::AllErrors() const -> std::vector<std::string> {
    auto errors = std::vector<std::string>();
    for (const auto& [name, project] : Projects) {
        if (!project) errors.push_back(project.error());
    }
    return errors;
}
