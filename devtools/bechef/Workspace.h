#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <optional>
#include <expected>

#include "BeShaderTools.h"

struct SourceFile {
    std::filesystem::path Dir;
    std::filesystem::path Path;
};

struct ShaderData {
    std::string Source;
    std::optional<BeShaderTools::ParsedShader> Shader;
    std::vector<BeShaderTools::ParsedMaterial> Materials;
};

struct SchemeEntry {
    std::filesystem::path File;
    BeShaderTools::ParsedMaterial Material;
};

struct VisibleSchemes {
    std::unordered_map<std::string, SchemeEntry> Schemes;
};

struct ResolvedBind {
    BeShaderTools::ParsedBind Bind;
    const SchemeEntry* Scheme = nullptr;
};

struct ShaderFile {
    std::filesystem::path Path;
    std::expected<ShaderData, std::string> Data;
    std::expected<std::vector<ResolvedBind>, std::string> Binds;
};

struct Project {
    std::string Name;
    std::filesystem::path Dir;
    std::vector<std::string> Dependencies;
    std::vector<std::string> LocalShaderDirs;
    std::vector<std::string> LocalAssetDirs;

    std::vector<ShaderFile> LocalShaders;
    std::vector<SourceFile> LocalAssetFiles;

    std::expected<std::vector<const Project*>, std::string> Scope;
    std::expected<VisibleSchemes, std::string> Schemes;
    std::expected<std::vector<const ShaderFile*>, std::string> FlatShaders;
};

struct WorkspaceConfig {
    std::filesystem::path Root;
    std::unordered_set<std::string> Modules;
    std::unordered_set<std::string> Apps;
    std::vector<std::string> Ignores;

    auto IsMember(const std::string& name) const -> bool {
        return Modules.contains(name) || Apps.contains(name);
    }
};

using ProjectOrError = std::expected<Project, std::string>;

struct Workspace {
    static auto Get() -> Workspace&;

    WorkspaceConfig Config;
    std::map<std::string, ProjectOrError> Projects;

    auto AllProjects() const -> std::vector<const Project*>;
    auto AppProjects() const -> std::vector<const Project*>;
    auto FindProject(const std::string& name) const -> const ProjectOrError*;
    auto FindOwningProject(const std::filesystem::path& file) const -> std::expected<const Project*, std::string>;
    auto AllErrors() const -> std::vector<std::string>;
};
