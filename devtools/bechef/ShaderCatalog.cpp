#include "ShaderCatalog.h"

#include <format>
#include <unordered_set>
#include <algorithm>

static auto ParseShaderFile(const std::filesystem::path& path, const std::string& owningProject) -> std::expected<ShaderFile, std::string> {
    auto file = ShaderFile();
    file.Path = path;
    file.OwningProject = owningProject;
    file.Source = BeShaderTools::ReadFile(path);

    if (file.Source.find("@be-shader") != std::string::npos) {
        auto shader = BeShaderTools::ParseShader(file.Source);
        if (!shader) {
            return std::unexpected(std::format("{}: @be-shader -> {}", path.string(), shader.error()));
        }
        file.Shader = std::move(*shader);
    }

    auto materials = BeShaderTools::ParseMaterials(file.Source);
    if (!materials) {
        return std::unexpected(std::format("{}: @be-material -> {}", path.string(), materials.error()));
    }
    file.Materials = std::move(*materials);

    return file;
}

auto BuildShaderCatalog(const Workspace& ws) -> std::expected<ShaderCatalog, std::string> {
    auto members = std::vector<std::string>(ws.Modules.begin(), ws.Modules.end());
    members.insert(members.end(), ws.Apps.begin(), ws.Apps.end());
    std::ranges::sort(members);

    auto catalog = ShaderCatalog();
    for (const auto& member : members) {
        const auto project = LoadProject(ws, member);
        if (!project) {
            return std::unexpected(project.error());
        }

        auto collectDirs = std::vector<std::string>(project->ShaderDirs);
        collectDirs.insert(collectDirs.end(), project->AssetDirs.begin(), project->AssetDirs.end()); // todo: shaders may be separated from assets, then remove this

        const auto sources = CollectProjectFiles(ws, *project, collectDirs);
        if (!sources) {
            return std::unexpected(sources.error());
        }

        for (const auto& source : *sources) {
            if (source.Path.extension() != ".hlsl") continue;

            auto file = ParseShaderFile(source.Path, member);
            if (!file) {
                return std::unexpected(file.error());
            }
            catalog.Files.push_back(std::move(*file));
        }
    }

    return catalog;
}

auto ScopeForProject(const Workspace& ws, const ShaderCatalog& catalog, const std::string& targetName) -> std::expected<SchemeScope, std::string> {
    const auto projects = ResolveProjects(ws, targetName);
    if (!projects) {
        return std::unexpected(projects.error());
    }

    auto closure = std::unordered_set<std::string>();
    for (const auto& project : *projects) {
        closure.insert(project.Name);
    }

    auto scope = SchemeScope();
    for (const auto& file : catalog.Files) {
        if (!closure.contains(file.OwningProject)) continue;

        for (const auto& material : file.Materials) {
            const auto it = scope.Schemes.find(material.Name);
            if (it != scope.Schemes.end()) {
                auto msg = std::format("scheme name collision '{}':\n  {}\n  {}", material.Name, it->second.File.string(), file.Path.string());
                return std::unexpected(msg);
            }
            scope.Schemes.emplace(material.Name, SchemeEntry{ file.Path, material });
        }
    }

    return scope;
}

auto ResolveOwningProject(const Workspace& ws, const std::filesystem::path& file) -> std::expected<std::string, std::string> {
    auto ec = std::error_code();
    const auto absolute = std::filesystem::weakly_canonical(std::filesystem::absolute(file), ec);
    const auto root = std::filesystem::weakly_canonical(ws.Root, ec);

    const auto relativePath = absolute.lexically_relative(root);
    if (relativePath.empty() || *relativePath.begin() == "..") {
        return std::unexpected(std::format("'{}' is outside the workspace at {}", file.string(), root.string()));
    }

    const auto name = relativePath.begin()->string();
    if (!ws.IsMember(name)) {
        return std::unexpected(std::format("'{}' belongs to '{}', which is not a project in workspace.bechef", file.string(), name));
    }
    return name;
}
