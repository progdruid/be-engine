#include "Deploy.h"

#include <print>
#include <format>
#include <fstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <algorithm>

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

auto CollectAssets(const Workspace& ws, const std::vector<Project>& projects) -> std::expected<DeployFiles, std::string> {
    auto collected = DeployFiles();

    for (const auto& project : projects) {
        const auto sources = CollectProjectFiles(ws, project, project.AssetDirs);
        if (!sources) {
            return std::unexpected(sources.error());
        }

        for (const auto& dir : project.AssetDirs) {
            collected.SourceDirs.push_back(ws.Root / project.Name / dir);
        }

        for (const auto& source : *sources) {
            const auto dstRel = std::filesystem::relative(source.Path, source.Dir);
            if (IsIgnored(ws, dstRel)) continue;
            collected.Entries.push_back({ source.Path, dstRel });
            collected.Inputs.push_back(source.Path);
        }
    }

    return collected;
}

auto CollectModuleShaders(const Workspace& ws, const std::vector<Project>& projects) -> std::expected<DeployFiles, std::string> {
    auto collected = DeployFiles();
    auto claimed = std::unordered_map<std::string, std::filesystem::path>();

    for (const auto& project : projects) {
        const auto sources = CollectProjectFiles(ws, project, project.ShaderDirs);
        if (!sources) {
            return std::unexpected(sources.error());
        }

        for (const auto& source : *sources) {
            const auto base = source.Path.filename().string();
            const auto it = claimed.find(base);
            if (it != claimed.end()) {
                return std::unexpected(std::format("module-shader name collision '{}':\n  {}\n  {}",
                    base, it->second.string(), source.Path.string()));
            }
            claimed.emplace(base, source.Path);
            collected.Entries.push_back({ source.Path, base });
            collected.Inputs.push_back(source.Path);
        }
    }

    return collected;
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

    const auto assets = CollectAssets(ws, *projects);
    if (!assets) {
        return std::unexpected(assets.error());
    }

    const auto shaders = CollectModuleShaders(ws, *projects);
    if (!shaders) {
        return std::unexpected(shaders.error());
    }

    auto inputs = std::vector<std::filesystem::path>{ ws.Root / "workspace.bechef" };
    for (const auto& project : *projects) {
        inputs.push_back(ws.Root / project.Name / "project.bechef");
    }
    inputs.insert(inputs.end(), assets->Inputs.begin(), assets->Inputs.end());
    inputs.insert(inputs.end(), shaders->Inputs.begin(), shaders->Inputs.end());

    const auto assetsOut = out / "assets";
    if (mode == Mode::Symlink && assets->SourceDirs.size() == 1) {
        auto r = SymlinkWholeDir(assets->SourceDirs.front(), assetsOut);
        if (!r) {
            return r;
        }
    }
    else {
        if (mode == Mode::Symlink && assets->SourceDirs.size() > 1) {
            std::println(stderr, "bechef: multiple asset dirs for '{}', per-file symlinking", app);
        }
        auto r = Materialize(assets->Entries, assetsOut, mode, true);
        if (!r) {
            return r;
        }
    }

    if (!shaders->Entries.empty()) {
        auto r = Materialize(shaders->Entries, out / "module-shaders", mode, true);
        if (!r) {
            return r;
        }
    }

    if (depfile) {
        WriteDepfile(*depfile, inputs);
    }
    return {};
}
