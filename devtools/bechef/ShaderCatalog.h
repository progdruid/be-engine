#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <filesystem>
#include <expected>

#include "BeShaderTools.h"
#include "Workspace.h"

struct ShaderFile {
    std::filesystem::path Path;
    std::string OwningProject;
    std::string Source;
    std::optional<BeShaderTools::ParsedShader> Shader;
    std::vector<BeShaderTools::ParsedMaterial> Materials;
};

struct SchemeEntry {
    std::filesystem::path File;
    BeShaderTools::ParsedMaterial Material;
};

struct ShaderCatalog {
    std::vector<ShaderFile> Files;
};

struct SchemeScope {
    std::unordered_map<std::string, SchemeEntry> Schemes;
};

auto BuildShaderCatalog(const Workspace& ws) -> std::expected<ShaderCatalog, std::string>;
auto ScopeForProject(const Workspace& ws, const ShaderCatalog& catalog, const std::string& targetName) -> std::expected<SchemeScope, std::string>;
auto ResolveOwningProject(const Workspace& ws, const std::filesystem::path& file) -> std::expected<std::string, std::string>;
