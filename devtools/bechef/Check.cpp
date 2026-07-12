#include "Check.h"

#include <print>
#include <format>
#include <vector>

#include "Deploy.h"
#include "ShaderCatalog.h"
#include "ShaderBoilerplate.h"

auto Check(const Workspace& ws) -> std::expected<void, std::string> {
    auto errors = std::vector<std::string>();

    auto members = std::vector<std::string>(ws.Modules.begin(), ws.Modules.end());
    members.insert(members.end(), ws.Apps.begin(), ws.Apps.end());

    for (const auto& member : members) {
        const auto project = LoadProject(ws, member);
        if (!project) { errors.push_back(project.error()); continue; }

        for (const auto& dep : project->Depends) {
            if (!ws.IsMember(dep)) errors.push_back(std::format("project '{}': depends on unknown '{}'", member, dep));
        }
        for (const auto& shaderDir : project->ShaderDirs) {
            if (!std::filesystem::is_directory(ws.Root / member / shaderDir)) errors.push_back(std::format("project '{}': shader dir '{}' not found", member, shaderDir));
        }
        for (const auto& assetDir : project->AssetDirs) {
            if (!std::filesystem::is_directory(ws.Root / member / assetDir)) errors.push_back(std::format("project '{}': asset dir '{}' not found", member, assetDir));
        }
    }

    for (const auto& app : ws.Apps) {
        const auto projects = ResolveProjects(ws, app);
        if (!projects) { errors.push_back(projects.error()); continue; }

        const auto shaders = CollectShaders(ws, *projects);
        if (!shaders) errors.push_back(shaders.error());
    }

    const auto catalog = BuildShaderCatalog(ws);
    if (!catalog) {
        errors.push_back(catalog.error());
    }
    else {
        for (const auto& member : members) {
            const auto scope = ScopeForProject(ws, *catalog, member);
            if (!scope) { errors.push_back(scope.error()); continue; }

            for (const auto& file : catalog->Files) {
                if (file.OwningProject != member) continue;

                const auto valid = ValidateShaderFile(file, *scope);
                if (!valid) errors.push_back(std::format("{}: {}", file.Path.string(), valid.error()));
            }
        }
    }

    if (!errors.empty()) {
        auto joined = std::string();
        for (const auto& e : errors) joined += "\n  " + e;
        return std::unexpected(std::format("workspace has {} problem(s):{}", errors.size(), joined));
    }
    std::println("bechef: workspace OK ({} projects)", members.size());
    return {};
}
