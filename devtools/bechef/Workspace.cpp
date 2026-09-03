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
        if (project && project->Kind == ProjectKind::App) apps.push_back(&*project);
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

    const Project* best = nullptr;
    auto bestLength = size_t{ 0 };
    for (const auto& [name, entry] : Projects) {
        if (!entry) continue;

        const auto dir = std::filesystem::weakly_canonical(entry->Dir, ec);
        const auto relative = absolute.lexically_relative(dir);
        if (relative.empty() || *relative.begin() == "..") continue;

        const auto length = dir.string().size();
        if (length > bestLength) {
            best = &*entry;
            bestLength = length;
        }
    }

    if (!best) {
        return std::unexpected(std::format("'{}' is not inside any known project", file.string()));
    }
    return best;
}

auto Workspace::AllErrors() const -> std::vector<std::string> {
    auto errors = std::vector<std::string>();
    for (const auto& [name, project] : Projects) {
        if (!project) errors.push_back(project.error());
    }
    return errors;
}
