#include "Load.h"

#include <format>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "BeShaderTools.h"
#include "CommandGrammar.h"
#include "Workspace.h"
#include "Try.h"

auto FindRoot(std::filesystem::path dir) -> std::optional<std::filesystem::path> {
    for (auto cur = dir; ; cur = cur.parent_path()) {
        if (std::filesystem::exists(cur / "workspace.bechef")) {
            return cur;
        }
        if (cur == cur.parent_path()) {
            return std::nullopt;
        }
    }
}

static auto LoadWorkspaceConfig(const std::filesystem::path& rootDir) -> std::expected<WorkspaceConfig, std::string> {
    const auto file = rootDir / "workspace.bechef";
    if (!std::filesystem::exists(file)) {
        return std::unexpected(std::format("no workspace.bechef in {}", rootDir.string()));
    }

    static const auto grammar = CommandGrammar{ {
        { .Verb = "roots",  .AllowPositionals = true },
        { .Verb = "ignore", .AllowPositionals = true },
    } };

    auto ws = WorkspaceConfig();
    ws.Root = rootDir;
    for (const auto& command : ParseRawCommands(BeShaderTools::ReadFile(file))) {
        bechef_try(const auto parsed, grammar.Parse(command), "workspace.bechef: {}");

        const auto& values = parsed.Positionals;
        if (command.Verb == "roots")        ws.Roots.insert(ws.Roots.end(), values.begin(), values.end());
        else if (command.Verb == "ignore")  ws.Ignores.insert(ws.Ignores.end(), values.begin(), values.end());
    }
    if (ws.Roots.empty()) ws.Roots.push_back(".");
    return ws;
}

static auto LoadProject(const std::filesystem::path& dir, const std::string& name) -> std::expected<Project, std::string> {
    const auto file = dir / "project.bechef";

    static const auto grammar = CommandGrammar{ {
        { .Verb = "kind",    .AllowPositionals = true },
        { .Verb = "depends", .AllowPositionals = true },
        { .Verb = "shaders", .AllowPositionals = true },
        { .Verb = "assets",  .AllowPositionals = true },
    } };

    auto project = Project();
    project.Name = name;
    project.Dir = dir;
    for (const auto& command : ParseRawCommands(BeShaderTools::ReadFile(file))) {
        bechef_try(const auto parsed, grammar.Parse(command), "{}/project.bechef: {}", name);

        const auto& values = parsed.Positionals;
        if (command.Verb == "kind") {
            if (values.size() != 1 || (values[0] != "app" && values[0] != "module")) {
                return std::unexpected(std::format("{}/project.bechef: kind must be 'app' or 'module'", name));
            }
            project.Kind = values[0] == "app" ? ProjectKind::App : ProjectKind::Module;
        }
        else if (command.Verb == "depends")  project.Dependencies.insert(project.Dependencies.end(), values.begin(), values.end());
        else if (command.Verb == "shaders")  project.LocalShaderDirs.insert(project.LocalShaderDirs.end(), values.begin(), values.end());
        else if (command.Verb == "assets")   project.LocalAssetDirs.insert(project.LocalAssetDirs.end(), values.begin(), values.end());
    }
    return project;
}

static auto MatchGlob(std::string_view pattern, std::string_view text) -> bool {
    constexpr auto none = std::string_view::npos;
    size_t pi = 0, si = 0, star = none, mark = 0;
    while (si < text.size()) {
        if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == text[si])) {
            pi++;
            si++;
        } else if (pi < pattern.size() && pattern[pi] == '*') {
            star = pi++;
            mark = si;
        } else if (star != none) {
            pi = star + 1;
            si = ++mark;
        } else {
            return false;
        }
    }
    while (pi < pattern.size() && pattern[pi] == '*') {
        pi++;
    }
    return pi == pattern.size();
}

static auto IsIgnored(const std::filesystem::path& rel, const std::vector<std::string>& ignore) -> bool {
    for (const auto& component : rel) {
        for (const auto& pattern : ignore) {
            if (MatchGlob(pattern, component.string())) return true;
        }
    }
    return false;
}

static auto CollectProjectFiles(
    const Project& project,
    const std::vector<std::string>& dirs,
    const std::vector<std::string>& ignore
) -> std::expected<std::vector<SourceFile>, std::string> {
    auto files = std::vector<SourceFile>();

    for (const auto& dir : dirs) {
        const auto srcDir = project.Dir / dir;
        if (!std::filesystem::is_directory(srcDir)) {
            return std::unexpected(std::format("project '{}': dir '{}' not found", project.Name, srcDir.string()));
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (!entry.is_regular_file()) continue;
            if (IsIgnored(std::filesystem::relative(entry.path(), srcDir), ignore)) continue;
            files.push_back({ srcDir, entry.path() });
        }
    }

    std::ranges::sort(files, {}, &SourceFile::Path);
    return files;
}

static auto ParseShader(const std::filesystem::path& path) -> std::expected<ShaderData, std::string> {
    auto data = ShaderData();
    data.Source = BeShaderTools::ReadFile(path);

    if (data.Source.find("@be-shader") != std::string::npos) {
        bechef_try(data.Shader, BeShaderTools::ParseShader(data.Source), "@be-shader -> {}");
    }

    bechef_try(data.Materials, BeShaderTools::ParseMaterials(data.Source), "@be-material -> {}");

    return data;
}

static auto ParseShaders(const std::string& collection, const std::vector<SourceFile>& files) -> std::vector<ShaderFile> {
    auto shaders = std::vector<ShaderFile>();

    for (const auto& source : files) {
        shaders.push_back({ collection, source.Path, ParseShader(source.Path) });
    }
    return shaders;
}

static auto ScopeInto(
    const Workspace& workspace,
    const std::string& name,
    std::unordered_set<std::string>& done,
    std::unordered_set<std::string>& stack,
    std::vector<const Project*>& order
) -> std::expected<void, std::string> {
    if (done.contains(name)) return {};
    if (stack.contains(name)) return std::unexpected(std::format("dependency cycle at '{}'", name));

    const auto* entry = workspace.FindProject(name);
    if (!entry) return std::unexpected(std::format("depends on unknown '{}'", name));
    if (!*entry) return std::unexpected(entry->error());

    stack.insert(name);
    for (const auto& dep : (*entry)->Dependencies) {
        const auto resolved = ScopeInto(workspace, dep, done, stack, order);
        if (!resolved) return resolved;
    }
    stack.erase(name);

    done.insert(name);
    order.push_back(&**entry);
    return {};
}

static auto BuildScope(const Workspace& workspace, const Project& project) -> std::expected<std::vector<const Project*>, std::string> {
    auto order = std::vector<const Project*>();
    auto done = std::unordered_set<std::string>();
    auto stack = std::unordered_set<std::string>();

    const auto resolved = ScopeInto(workspace, project.Name, done, stack, order);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }
    return order;
}

static auto BuildVisibleSchemes(const Project& project) -> std::expected<VisibleSchemes, std::string> {
    bechef_try(const auto& scope, project.Scope);

    auto visible = VisibleSchemes();
    for (const auto* dependency : scope) {
        for (const auto& shader : dependency->LocalShaders) {
            if (!shader.Data) continue;

            for (const auto& material : shader.Data->Materials) {
                const auto it = visible.Schemes.find(material.Name);
                if (it != visible.Schemes.end()) {
                    return std::unexpected(std::format("scheme name collision '{}':\n  {}\n  {}",
                        material.Name, it->second.File.string(), shader.Path.string()));
                }
                visible.Schemes.emplace(material.Name, SchemeEntry{ shader.Collection, shader.Path, material });
            }
        }
    }
    return visible;
}

static auto ResolveBinds(const ShaderFile& shader, const VisibleSchemes& visible) -> std::expected<std::vector<ResolvedBind>, std::string> {
    auto resolved = std::vector<ResolvedBind>();
    if (!shader.Data || !shader.Data->Shader) {
        return resolved;
    }

    for (const auto& bind : shader.Data->Shader->Binds) {
        const auto it = visible.Schemes.find(bind.Scheme);
        if (it == visible.Schemes.end()) {
            return std::unexpected(std::format("bind '{}': scheme '{}' is not declared in the project's scope", bind.Link, bind.Scheme));
        }
        resolved.push_back({ bind, &it->second });
    }

    std::ranges::sort(resolved, {}, [](const ResolvedBind& resolvedBind) { return resolvedBind.Bind.Slot; });
    return resolved;
}

static auto BuildScopedShaders(const Project& project) -> std::expected<std::vector<const ShaderFile*>, std::string> {
    bechef_try(const auto& scope, project.Scope);

    auto scoped = std::vector<const ShaderFile*>();
    auto claimed = std::unordered_map<std::string, std::filesystem::path>();

    for (const auto* dependency : scope) {
        for (const auto& shader : dependency->LocalShaders) {
            const auto key = shader.Collection + "/" + shader.Path.filename().string();
            const auto it = claimed.find(key);
            if (it != claimed.end()) {
                return std::unexpected(std::format("shader name collision '{}':\n  {}\n  {}",
                    key, it->second.string(), shader.Path.string()));
            }
            claimed.emplace(key, shader.Path);
            scoped.push_back(&shader);
        }
    }
    return scoped;
}

static auto LoadProjectEntry(
    const std::filesystem::path& dir,
    const std::string& name,
    const std::vector<std::string>& ignore
) -> ProjectOrError {
    bechef_try(auto project, LoadProject(dir, name));
    bechef_try(const auto shaderFiles, CollectProjectFiles(project, project.LocalShaderDirs, ignore));
    bechef_try(project.LocalAssetFiles, CollectProjectFiles(project, project.LocalAssetDirs, ignore));
    project.LocalShaders = ParseShaders(name, shaderFiles);
    return project;
}

struct DiscoveredProject {
    std::string Name;
    std::filesystem::path Dir;
};

static auto DiscoverProjects(const WorkspaceConfig& config) -> std::vector<DiscoveredProject> {
    auto found = std::map<std::string, DiscoveredProject>();

    auto skip = [&](const std::filesystem::path& name) {
        const auto s = name.string();
        if (!s.empty() && s.front() == '.') return true;
        for (const auto& pattern : config.Ignores) {
            if (MatchGlob(pattern, s)) return true;
        }
        return false;
    };

    for (const auto& root : config.Roots) {
        const auto rootDir = std::filesystem::weakly_canonical(config.Root / root);
        if (!std::filesystem::is_directory(rootDir)) continue;

        auto it = std::filesystem::recursive_directory_iterator(rootDir);
        const auto end = std::filesystem::recursive_directory_iterator();
        for (; it != end; ++it) {
            if (it->is_directory() && skip(it->path().filename())) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_regular_file() || it->path().filename() != "project.bechef") continue;

            const auto dir = it->path().parent_path();
            found.insert({ dir.filename().string(), { dir.filename().string(), dir } });
        }
    }

    auto projects = std::vector<DiscoveredProject>();
    for (auto& [name, project] : found) projects.push_back(std::move(project));
    return projects;
}

auto LoadWorkspace(const std::filesystem::path& rootDir) -> std::expected<void, std::string> {
    auto workspace = Workspace();

    bechef_try(workspace.Config, LoadWorkspaceConfig(rootDir));

    for (const auto& discovered : DiscoverProjects(workspace.Config)) {
        workspace.Projects.emplace(discovered.Name, LoadProjectEntry(discovered.Dir, discovered.Name, workspace.Config.Ignores));
    }

    for (auto& [name, entry] : workspace.Projects) {
        if (!entry) continue;
        entry->Scope = BuildScope(workspace, *entry);
    }

    for (auto& [name, entry] : workspace.Projects) {
        if (!entry) continue;
        entry->Schemes = BuildVisibleSchemes(*entry);
        entry->ScopedShaders = BuildScopedShaders(*entry);
    }

    for (auto& [name, entry] : workspace.Projects) {
        if (!entry || !entry->Schemes) continue;
        for (auto& shader : entry->LocalShaders) {
            shader.Binds = ResolveBinds(shader, *entry->Schemes);
        }
    }

    Workspace::Get() = std::move(workspace);
    return {};
}

auto ReloadWorkspace() -> std::expected<void, std::string> {
    return LoadWorkspace(Workspace::Get().Config.Root);
}
