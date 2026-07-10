#include "Actions.h"

#include <print>
#include <format>
#include <fstream>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <algorithm>

struct DeployEntry {
    std::filesystem::path Src;
    std::filesystem::path DstRel;
};

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
            if (!std::filesystem::is_directory(srcDir)) {
                return std::unexpected(std::format("project '{}': asset dir '{}' not found", project.Name, srcDir.string()));
            }
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

auto Deploy(
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
    if (!assetDirs) {
        return std::unexpected(assetDirs.error());
    }

    auto r = CollectModuleShaders(ws, *projects, shaderEntries, inputs);
    if (!r) {
        return std::unexpected(r.error());
    }

    const auto assetsOut = out / "assets";
    if (mode == Mode::Symlink && assetDirs->size() == 1) {
        auto r = SymlinkWholeDir(assetDirs->front(), assetsOut);
        if (!r) {
            return r;
        }
    }
    else {
        if (mode == Mode::Symlink && assetDirs->size() > 1) {
            std::println(stderr, "bechef: multiple asset dirs for '{}', per-file symlinking", app);
        }
        auto r = Materialize(assetEntries, assetsOut, mode, true);
        if (!r) {
            return r;
        }
    }

    if (!shaderEntries.empty()) {
        auto r = Materialize(shaderEntries, out / "module-shaders", mode, true);
        if (!r) {
            return r;
        }
    }

    if (depfile) {
        WriteDepfile(*depfile, inputs);
    }
    return {};
}

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
