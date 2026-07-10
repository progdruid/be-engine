#include "Workspace.h"

#include <format>
#include <unordered_set>

#include "BeShaderTools.h"
#include "CommandGrammar.h"

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

auto LoadWorkspace(const std::filesystem::path& rootDir) -> std::expected<Workspace, std::string> {
    const auto file = rootDir / "workspace.bechef";
    if (!std::filesystem::exists(file)) {
        return std::unexpected(std::format("no workspace.bechef in {}", rootDir.string()));
    }

    static const auto grammar = CommandGrammar{ {
        { .Verb = "modules", .AllowPositionals = true },
        { .Verb = "apps",    .AllowPositionals = true },
        { .Verb = "ignore",  .AllowPositionals = true },
    } };

    auto ws = Workspace();
    ws.Root = rootDir;
    for (const auto& command : ParseRawCommands(BeShaderTools::ReadFile(file))) {
        const auto parsed = grammar.Parse(command);
        if (!parsed) return std::unexpected(std::format("workspace.bechef: {}", parsed.error()));

        const auto& values = parsed->Positionals;
        if (command.Verb == "modules")     ws.Modules.insert(values.begin(), values.end());
        else if (command.Verb == "apps")   ws.Apps.insert(values.begin(), values.end());
        else if (command.Verb == "ignore") ws.Ignore.insert(ws.Ignore.end(), values.begin(), values.end());
    }
    return ws;
}

auto LoadProject(const Workspace& ws, const std::string& name) -> std::expected<Project, std::string> {
    const auto file = ws.Root / name / "project.bechef";
    if (!std::filesystem::exists(file)) {
        return std::unexpected(std::format("project '{}': missing {}", name, file.string()));
    }

    static const auto grammar = CommandGrammar{ {
        { .Verb = "depends", .AllowPositionals = true },
        { .Verb = "shaders", .AllowPositionals = true },
        { .Verb = "assets",  .AllowPositionals = true },
    } };

    auto project = Project();
    project.Name = name;
    for (const auto& command : ParseRawCommands(BeShaderTools::ReadFile(file))) {
        const auto parsed = grammar.Parse(command);
        if (!parsed) return std::unexpected(std::format("{}/project.bechef: {}", name, parsed.error()));

        const auto& values = parsed->Positionals;
        if (command.Verb == "depends")      project.Depends.insert(project.Depends.end(), values.begin(), values.end());
        else if (command.Verb == "shaders") project.ShaderDirs.insert(project.ShaderDirs.end(), values.begin(), values.end());
        else if (command.Verb == "assets")  project.AssetDirs.insert(project.AssetDirs.end(), values.begin(), values.end());
    }
    return project;
}

static auto ResolveInto(
    const Workspace& ws,
    const std::string& name,
    std::unordered_set<std::string>& done,
    std::unordered_set<std::string>& stack,
    std::vector<Project>& order
) -> std::expected<void, std::string> {
    if (done.contains(name)) return {};
    if (!ws.IsMember(name)) return std::unexpected(std::format("unknown project '{}'", name));
    if (stack.contains(name)) return std::unexpected(std::format("dependency cycle at '{}'", name));

    auto project = LoadProject(ws, name);
    if (!project) return std::unexpected(project.error());

    stack.insert(name);
    for (const auto& dep : project->Depends) {
        if (auto r = ResolveInto(ws, dep, done, stack, order); !r) return r;
    }
    stack.erase(name);

    done.insert(name);
    order.push_back(std::move(*project));
    return {};
}

auto ResolveProjects(const Workspace& ws, const std::string& app) -> std::expected<std::vector<Project>, std::string> {
    auto order = std::vector<Project>();
    auto done = std::unordered_set<std::string>();
    auto stack = std::unordered_set<std::string>();
    if (auto r = ResolveInto(ws, app, done, stack, order); !r) return std::unexpected(r.error());
    return order;
}
