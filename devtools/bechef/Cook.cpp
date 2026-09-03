#include "Cook.h"

#include <print>
#include <format>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <algorithm>

#include "Workspace.h"
#include "Verify.h"
#include "Try.h"

struct CookEntry {
    std::filesystem::path Src;
    std::filesystem::path DstRel;
};

struct CookFiles {
    std::vector<CookEntry> Entries;
    std::vector<std::filesystem::path> SourceDirs;
};

static auto CollectAssets(const std::vector<const Project*>& projects) -> CookFiles {
    auto collected = CookFiles();

    for (const auto* project : projects) {
        for (const auto& dir : project->LocalAssetDirs) {
            collected.SourceDirs.push_back(project->Dir / dir);
        }

        for (const auto& source : project->LocalAssetFiles) {
            const auto dstRel = std::filesystem::relative(source.Path, source.Dir);
            collected.Entries.push_back({ source.Path, dstRel });
        }
    }

    return collected;
}

static auto CollectShaders(const std::vector<const ShaderFile*>& scopedShaders) -> CookFiles {
    auto collected = CookFiles();

    for (const auto* shader : scopedShaders) {
        collected.Entries.push_back({ shader->Path, std::filesystem::path(shader->Collection) / shader->Path.filename() });
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

static auto Materialize(const std::vector<CookEntry>& entries, const std::filesystem::path& destRoot, Mode mode, bool prune) -> std::expected<void, std::string> {
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

auto Cook(
    const std::string& app,
    const std::filesystem::path& out,
    Mode mode
) -> std::expected<void, std::string> {

    bechef_try(const auto* project, VerifyApp(app));
    bechef_try(const auto& scopedShaders, project->ScopedShaders);

    const auto& closure = *project->Scope;
    const auto assets = CollectAssets(closure);
    const auto shaders = CollectShaders(scopedShaders);

    const auto assetsOut = out / "assets";
    if (mode == Mode::Symlink && assets.SourceDirs.size() == 1) {
        auto r = SymlinkWholeDir(assets.SourceDirs.front(), assetsOut);
        if (!r) {
            return r;
        }
    }
    else if (!assets.SourceDirs.empty()) {
        if (mode == Mode::Symlink) {
            std::println(stderr, "bechef: multiple asset dirs for '{}', per-file symlinking", app);
        }
        auto r = Materialize(assets.Entries, assetsOut, mode, true);
        if (!r) {
            return r;
        }
    }

    if (!shaders.Entries.empty()) {
        auto r = Materialize(shaders.Entries, out / "shaders", mode, true);
        if (!r) {
            return r;
        }
    }

    return {};
}
