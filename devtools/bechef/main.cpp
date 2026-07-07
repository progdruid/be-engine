#include <print>
#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <optional>
#include <expected>
#include <system_error>
#include <algorithm>
#include <chrono>
#include <thread>

#include "BeShaderTools.h"

enum class Mode { Copy, Symlink };

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

struct DeployEntry {
    std::filesystem::path Src;
    std::filesystem::path DstRel;
};

using KeyedLines = std::vector<std::pair<std::string, std::vector<std::string>>>;

static auto ParseKeyedLines(std::string_view text) -> KeyedLines {
    auto out = KeyedLines();
    for (const auto rawLine : BeShaderTools::Split(text, "\n")) {
        const auto line = BeShaderTools::Trim(rawLine.substr(0, rawLine.find('#')), " \t\r");
        if (line.empty()) continue;

        const auto tokens = BeShaderTools::Split(line, " \t");
        auto values = std::vector<std::string>(tokens.begin() + 1, tokens.end());
        out.emplace_back(std::string(tokens[0]), std::move(values));
    }
    return out; 
}

static auto LoadWorkspace(const std::filesystem::path& rootDir) -> std::expected<Workspace, std::string> {
    const auto file = rootDir / "workspace.bechef";
    if (!std::filesystem::exists(file)) {
        return std::unexpected(std::format("no workspace.bechef in {}", rootDir.string()));
    }
    
    const auto fileText = BeShaderTools::ReadFile(file);
    const auto lines = ParseKeyedLines(fileText);
    
    auto ws = Workspace();
    ws.Root = rootDir;
    for (const auto& [key, values] : lines) {
        if (key == "modules")     ws.Modules.insert(values.begin(), values.end());
        else if (key == "apps")   ws.Apps.insert(values.begin(), values.end());
        else if (key == "ignore") ws.Ignore.insert(ws.Ignore.end(), values.begin(), values.end());
        else                      return std::unexpected(std::format("workspace.bechef: unknown key '{}'", key));
    }
    return ws;
}

static auto LoadProject(const Workspace& ws, const std::string& name) -> std::expected<Project, std::string> {
    const auto file = ws.Root / name / "project.bechef";
    if (!std::filesystem::exists(file)) {
        return std::unexpected(std::format("project '{}': missing {}", name, file.string()));
    }

    auto project = Project();
    project.Name = name;
    for (const auto& [key, values] : ParseKeyedLines(BeShaderTools::ReadFile(file))) {
        if (key == "depends") project.Depends.insert(project.Depends.end(), values.begin(), values.end());
        else if (key == "shaders") project.ShaderDirs.insert(project.ShaderDirs.end(), values.begin(), values.end());
        else if (key == "assets") project.AssetDirs.insert(project.AssetDirs.end(), values.begin(), values.end());
        else return std::unexpected(std::format("{}/project.bechef: unknown key '{}'", name, key));
    }
    return project;
}

static auto ResolveInto(const Workspace& ws, const std::string& name,
                        std::unordered_set<std::string>& done, std::unordered_set<std::string>& stack,
                        std::vector<Project>& order) -> std::expected<void, std::string> {
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

static auto ResolveProjects(const Workspace& ws, const std::string& app) -> std::expected<std::vector<Project>, std::string> {
    auto order = std::vector<Project>();
    auto done = std::unordered_set<std::string>();
    auto stack = std::unordered_set<std::string>();
    if (auto r = ResolveInto(ws, app, done, stack, order); !r) return std::unexpected(r.error());
    return order;
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

static auto IsIgnored(const Workspace& ws, const std::filesystem::path& rel) -> bool {
    for (const auto& component : rel) {
        for (const auto& pattern : ws.Ignore) {
            if (MatchGlob(pattern, component.string())) return true;
        }
    }
    return false;
}

static auto CollectAssets(const Workspace& ws, const std::vector<Project>& projects,
                          std::vector<DeployEntry>& entries, std::vector<std::filesystem::path>& inputs)
    -> std::expected<std::vector<std::filesystem::path>, std::string> {
    auto sourceDirs = std::vector<std::filesystem::path>();
    for (const auto& project : projects) {
        for (const auto& rel : project.AssetDirs) {
            const auto srcDir = ws.Root / project.Name / rel;
            if (!std::filesystem::is_directory(srcDir)) return std::unexpected(std::format("project '{}': asset dir '{}' not found", project.Name, srcDir.string()));
            sourceDirs.push_back(srcDir);

            for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
                if (!entry.is_regular_file()) continue;
                const auto dstRel = std::filesystem::relative(entry.path(), srcDir);
                if (IsIgnored(ws, dstRel)) continue;
                entries.push_back({ entry.path(), dstRel });
                inputs.push_back(entry.path());
            }
        }
    }
    return sourceDirs;
}

static auto CollectModuleShaders(const Workspace& ws, const std::vector<Project>& projects,
                                 std::vector<DeployEntry>& entries, std::vector<std::filesystem::path>& inputs)
    -> std::expected<void, std::string> {
    auto claimed = std::unordered_map<std::string, std::filesystem::path>();
    for (const auto& project : projects) {
        for (const auto& rel : project.ShaderDirs) {
            const auto srcDir = ws.Root / project.Name / rel;
            if (!std::filesystem::is_directory(srcDir)) return std::unexpected(std::format("project '{}': shader dir '{}' not found", project.Name, srcDir.string()));

            for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
                if (!entry.is_regular_file()) continue;
                const auto base = entry.path().filename().string();
                if (const auto it = claimed.find(base); it != claimed.end()) {
                    return std::unexpected(std::format("module-shader name collision '{}':\n  {}\n  {}",
                        base, it->second.string(), entry.path().string()));
                }
                claimed.emplace(base, entry.path());
                entries.push_back({ entry.path(), base });
                inputs.push_back(entry.path());
            }
        }
    }
    return {};
}

static auto NeedsCopy(const std::filesystem::path& src, const std::filesystem::path& dst) -> bool {
    auto ec = std::error_code();
    if (!std::filesystem::exists(dst, ec)) return true;
    if (std::filesystem::file_size(src, ec) != std::filesystem::file_size(dst, ec)) return true;
    return std::filesystem::last_write_time(src, ec) > std::filesystem::last_write_time(dst, ec);
}

static auto Prune(const std::filesystem::path& destRoot, const std::unordered_set<std::string>& keep) -> void {
    auto ec = std::error_code();
    if (!std::filesystem::exists(destRoot, ec)) return;

    auto stale = std::vector<std::filesystem::path>();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(destRoot, ec)) {
        if (entry.is_directory(ec)) continue;
        if (!keep.contains(entry.path().lexically_normal().string())) stale.push_back(entry.path());
    }
    for (const auto& p : stale) std::filesystem::remove(p, ec);

    auto dirs = std::vector<std::filesystem::path>();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(destRoot, ec)) {
        if (entry.is_directory(ec)) dirs.push_back(entry.path());
    }
    std::ranges::sort(dirs, [](const std::filesystem::path& a, const std::filesystem::path& b) { return a.string().size() > b.string().size(); });
    for (const auto& d : dirs) {
        if (std::filesystem::is_empty(d, ec)) std::filesystem::remove(d, ec);
    }
}

static auto Materialize(const std::vector<DeployEntry>& entries, const std::filesystem::path& destRoot, Mode mode, bool prune) -> std::expected<void, std::string> {
    auto ec = std::error_code();
    if (mode == Mode::Copy && std::filesystem::is_symlink(destRoot, ec)) std::filesystem::remove(destRoot, ec);
    std::filesystem::create_directories(destRoot, ec);

    auto keep = std::unordered_set<std::string>();
    for (const auto& entry : entries) {
        const auto dst = destRoot / entry.DstRel;
        keep.insert(dst.lexically_normal().string());
        std::filesystem::create_directories(dst.parent_path(), ec);

        if (mode == Mode::Symlink) {
            std::filesystem::remove(dst, ec);
            ec.clear();
            std::filesystem::create_symlink(entry.Src, dst, ec);
            if (ec) {
                ec.clear();
                std::filesystem::copy_file(entry.Src, dst, std::filesystem::copy_options::overwrite_existing, ec);
                std::println(stderr, "bechef: could not symlink {}, copied instead", dst.string());
            }
        }
        else if (NeedsCopy(entry.Src, dst)) {
            std::filesystem::copy_file(entry.Src, dst, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) return std::unexpected(std::format("copy {} -> {}: {}", entry.Src.string(), dst.string(), ec.message()));
        }
    }

    if (prune) Prune(destRoot, keep);
    return {};
}

static auto SymlinkWholeDir(const std::filesystem::path& src, const std::filesystem::path& dst) -> std::expected<void, std::string> {
    auto ec = std::error_code();
    std::filesystem::remove_all(dst, ec);
    std::filesystem::create_directories(dst.parent_path(), ec);
    std::filesystem::create_directory_symlink(src, dst, ec);
    if (ec) return std::unexpected(std::format("symlink {} -> {}: {}", dst.string(), src.string(), ec.message()));
    return {};
}

static auto EscapeMake(const std::string& path) -> std::string {
    auto out = std::string();
    for (const auto c : path) {
        if (c == ' ' || c == ':') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

static auto WriteDepfile(const std::filesystem::path& depfile, const std::vector<std::filesystem::path>& inputs) -> void {
    auto target = depfile.string();
    if (target.ends_with(".d")) target.resize(target.size() - 2);

    auto os = std::ofstream(depfile);
    os << EscapeMake(target) << ":";
    for (const auto& input : inputs) os << " \\\n  " << EscapeMake(input.string());
    os << "\n";
}

static auto Deploy(
    const Workspace& ws, 
    const std::string& app, 
    const std::filesystem::path& out, 
    Mode mode,
    const std::optional<std::filesystem::path>& depfile
) -> std::expected<void, std::string> {
    
    if (!ws.Apps.contains(app)) {
        return std::unexpected(std::format("'{}' is not an app in workspace.bechef", app));
    }

    const auto projects = ResolveProjects(ws, app);
    if (!projects) {
        return std::unexpected(projects.error());
    }

    auto inputs = std::vector<std::filesystem::path>{ ws.Root / "workspace.bechef" };
    for (const auto& project : *projects) {
        inputs.push_back(ws.Root / project.Name / "project.bechef");
    }

    auto assetEntries = std::vector<DeployEntry>();
    auto shaderEntries = std::vector<DeployEntry>();

    const auto assetDirs = CollectAssets(ws, *projects, assetEntries, inputs);
    if (!assetDirs) return std::unexpected(assetDirs.error());

    auto r = CollectModuleShaders(ws, *projects, shaderEntries, inputs);
    if (!r) return std::unexpected(r.error());

    const auto assetsOut = out / "assets";
    if (mode == Mode::Symlink && assetDirs->size() == 1) {
        auto r = SymlinkWholeDir(assetDirs->front(), assetsOut); 
        if (!r) return r;
    }
    else {
        if (mode == Mode::Symlink && assetDirs->size() > 1) {
            std::println(stderr, "bechef: multiple asset dirs for '{}', per-file symlinking", app);
        }
        auto r = Materialize(assetEntries, assetsOut, mode, true); 
        if (!r) return r;
    }

    if (!shaderEntries.empty()) {
        auto r = Materialize(shaderEntries, out / "module-shaders", mode, true);
        if (!r) return r;
    }

    if (depfile) {
        WriteDepfile(*depfile, inputs);
    }
    return {};
}

static auto Check(const Workspace& ws) -> std::expected<void, std::string> {
    auto errors = std::vector<std::string>();

    auto members = std::vector<std::string>(ws.Modules.begin(), ws.Modules.end());
    members.insert(members.end(), ws.Apps.begin(), ws.Apps.end());

    for (const auto& name : members) {
        const auto project = LoadProject(ws, name);
        if (!project) { errors.push_back(project.error()); continue; }

        for (const auto& dep : project->Depends) {
            if (!ws.IsMember(dep)) errors.push_back(std::format("project '{}': depends on unknown '{}'", name, dep));
        }
        for (const auto& rel : project->ShaderDirs) {
            if (!std::filesystem::is_directory(ws.Root / name / rel)) errors.push_back(std::format("project '{}': shader dir '{}' not found", name, rel));
        }
        for (const auto& rel : project->AssetDirs) {
            if (!std::filesystem::is_directory(ws.Root / name / rel)) errors.push_back(std::format("project '{}': asset dir '{}' not found", name, rel));
        }
    }

    for (const auto& app : ws.Apps) {
        const auto projects = ResolveProjects(ws, app);
        if (!projects) { errors.push_back(projects.error()); continue; }

        auto entries = std::vector<DeployEntry>();
        auto inputs = std::vector<std::filesystem::path>();
        if (auto r = CollectModuleShaders(ws, *projects, entries, inputs); !r) errors.push_back(r.error());
    }

    if (!errors.empty()) {
        auto joined = std::string();
        for (const auto& e : errors) joined += "\n  " + e;
        return std::unexpected(std::format("workspace has {} problem(s):{}", errors.size(), joined));
    }
    std::println("bechef: workspace OK ({} projects)", members.size());
    return {};
}

static auto SnapshotDirs(const std::vector<std::filesystem::path>& dirs, const std::vector<std::filesystem::path>& files)
    -> std::map<std::string, std::filesystem::file_time_type> {
    auto snapshot = std::map<std::string, std::filesystem::file_time_type>();
    auto ec = std::error_code();
    for (const auto& dir : dirs) {
        if (!std::filesystem::is_directory(dir, ec)) continue;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec)) snapshot[entry.path().string()] = entry.last_write_time(ec);
        }
    }
    for (const auto& file : files) {
        if (std::filesystem::exists(file, ec)) snapshot[file.string()] = std::filesystem::last_write_time(file, ec);
    }
    return snapshot;
}

static auto Watch(const Workspace& ws, const std::string& app, const std::filesystem::path& out, Mode mode) -> std::expected<void, std::string> {
    if (auto r = Deploy(ws, app, out, mode, std::nullopt); !r) std::println(stderr, "bechef: {}", r.error());

    const auto projects = ResolveProjects(ws, app);
    if (!projects) return std::unexpected(projects.error());

    auto watchDirs = std::vector<std::filesystem::path>();
    auto watchFiles = std::vector<std::filesystem::path>{ ws.Root / "workspace.bechef" };
    for (const auto& project : *projects) {
        watchFiles.push_back(ws.Root / project.Name / "project.bechef");
        for (const auto& rel : project.ShaderDirs) watchDirs.push_back(ws.Root / project.Name / rel);
        for (const auto& rel : project.AssetDirs) watchDirs.push_back(ws.Root / project.Name / rel);
    }

    std::println("bechef: watching '{}' (Ctrl+C to stop)...", app);
    auto previous = SnapshotDirs(watchDirs, watchFiles);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto current = SnapshotDirs(watchDirs, watchFiles);
        if (current == previous) continue;

        previous = std::move(current);
        if (auto r = Deploy(ws, app, out, mode, std::nullopt); !r) std::println(stderr, "bechef: {}", r.error());
        else std::println("bechef: redeployed '{}'", app);
    }
}

static auto FindRoot(std::filesystem::path dir) -> std::optional<std::filesystem::path> {
    for (auto cur = dir; ; cur = cur.parent_path()) {
        if (std::filesystem::exists(cur / "workspace.bechef")) return cur;
        if (cur == cur.parent_path()) return std::nullopt;
    }
}

static auto ParseFlags(int argc, char* argv[]) -> std::unordered_map<std::string, std::string> {
    auto flags = std::unordered_map<std::string, std::string>();
    for (int i = 2; i < argc; i++) {
        const auto arg = std::string_view(argv[i]);
        if (!arg.starts_with("--")) {
            continue;
        }

        const auto key = std::string(arg.substr(2));
        const auto hasValue = i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("--");
        flags[key] = hasValue ? argv[++i] : "";
    }
    return flags;
}

static auto Usage() -> void {
    std::println(stderr, "Usage:");
    std::println(stderr, "  bechef deploy --app <name> --out <dir> [--mode copy|symlink] [--root <dir>] [--depfile <path>]");
    std::println(stderr, "  bechef watch  --app <name> --out <dir> [--mode copy|symlink] [--root <dir>]");
    std::println(stderr, "  bechef check  [--root <dir>]");
}

int main(int argc, char* argv[]) {
    if (argc < 2) { Usage(); return 1; }
    const auto command = std::string(argv[1]);
    const auto flags = ParseFlags(argc, argv);

    auto rootDir = std::filesystem::path();
    if (const auto it = flags.find("root"); it != flags.end()) {
        rootDir = it->second;
    } else if (const auto found = FindRoot(std::filesystem::current_path())) {
        rootDir = *found;
    } else {
        std::println(stderr, "bechef: no workspace.bechef found from {}", std::filesystem::current_path().string());
        return 1;
    }

    const auto workspace = LoadWorkspace(rootDir);
    if (!workspace) {
        std::println(stderr, "bechef: {}", workspace.error()); return 1;
    }

    const auto mode = flags.contains("mode") && flags.at("mode") == "symlink" ? Mode::Symlink : Mode::Copy;
    const auto app  = flags.contains("app") ? flags.at("app") : "";
    const auto out  = flags.contains("out") ? std::filesystem::path(flags.at("out")) : std::filesystem::path();

    auto result = std::expected<void, std::string>();
    if (command == "deploy") {
        if (app.empty() || out.empty()) {
            Usage(); 
            return 1;
        }
        
        auto depfile = std::optional<std::filesystem::path>();
        if (const auto it = flags.find("depfile"); it != flags.end()) {
            depfile = it->second;
        }
        
        result = Deploy(*workspace, app, out, mode, depfile);
    }
    else if (command == "watch") {
        if (app.empty() || out.empty()) {
            Usage(); 
            return 1;
        }
        
        result = Watch(*workspace, app, out, mode);
    }
    else if (command == "check") {
        result = Check(*workspace);
    }
    else {
        Usage(); 
        return 1;
    }

    if (!result) {
        std::println(stderr, "bechef: {}", result.error()); return 1;
    }
    
    return 0;
}
