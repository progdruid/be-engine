#include "Verify.h"

#include <format>
#include <vector>
#include <unordered_set>

static auto Problems(const std::vector<std::string>& errors) -> std::string {
    auto seen = std::unordered_set<std::string>();
    auto joined = std::string();
    auto count = 0;

    for (const auto& error : errors) {
        if (!seen.insert(error).second) continue;
        joined += "\n  " + error;
        count++;
    }
    return std::format("{} problem(s):{}", count, joined);
}

static auto ProjectErrors(const Project& project, std::vector<std::string>& errors) -> void {
    if (!project.Scope) {
        errors.push_back(std::format("project '{}': {}", project.Name, project.Scope.error()));
        return;
    }

    for (const auto& shader : project.LocalShaders) {
        if (!shader.Data) errors.push_back(std::format("{}: {}", shader.Path.string(), shader.Data.error()));
    }

    if (!project.Schemes) {
        errors.push_back(std::format("project '{}': {}", project.Name, project.Schemes.error()));
    }
    else {
        for (const auto& shader : project.LocalShaders) {
            if (!shader.Binds) errors.push_back(std::format("{}: {}", shader.Path.string(), shader.Binds.error()));
        }
    }

    if (!project.FlatShaders) {
        errors.push_back(std::format("project '{}': {}", project.Name, project.FlatShaders.error()));
    }
}

auto VerifyProject(const Project& project) -> std::expected<void, std::string> {
    auto errors = std::vector<std::string>();
    ProjectErrors(project, errors);

    if (!errors.empty()) return std::unexpected(Problems(errors));
    return {};
}

auto VerifyApp(const std::string& name) -> std::expected<const Project*, std::string> {
    const auto& workspace = Workspace::Get();
    if (!workspace.Config.Apps.contains(name)) {
        return std::unexpected(std::format("'{}' is not an app in workspace.bechef", name));
    }

    const auto* entry = workspace.FindProject(name);
    if (!entry) {
        return std::unexpected(std::format("'{}' has no project directory", name));
    }
    if (!*entry) {
        return std::unexpected(entry->error());
    }

    const auto& app = **entry;
    auto errors = std::vector<std::string>();

    if (!app.Scope) {
        errors.push_back(std::format("project '{}': {}", app.Name, app.Scope.error()));
        return std::unexpected(Problems(errors));
    }

    for (const auto* dependency : *app.Scope) {
        ProjectErrors(*dependency, errors);
    }

    if (!errors.empty()) return std::unexpected(Problems(errors));
    return &app;
}

auto VerifyWorkspace() -> std::expected<void, std::string> {
    const auto& workspace = Workspace::Get();

    auto errors = workspace.AllErrors();
    for (const auto* project : workspace.AllProjects()) {
        ProjectErrors(*project, errors);
    }

    if (!errors.empty()) return std::unexpected(Problems(errors));
    return {};
}
