#include "ShaderGen.h"

#include <print>
#include <format>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <system_error>
#include <algorithm>
#include <thread>
#include <chrono>

#include "ShaderCatalog.h"
#include "ShaderBoilerplate.h"

static auto TargetProjects(const Workspace& ws, const ShaderGenOptions& options) -> std::expected<std::vector<std::string>, std::string> {
    if (options.File) {
        const auto owner = ResolveOwningProject(ws, *options.File);
        if (!owner) {
            return std::unexpected(owner.error());
        }
        return std::vector<std::string>{ *owner };
    }

    if (options.Project) {
        if (!ws.IsMember(*options.Project)) {
            return std::unexpected(std::format("'{}' is not a project in workspace.bechef", *options.Project));
        }
        return std::vector<std::string>{ *options.Project };
    }

    auto members = std::vector<std::string>(ws.Modules.begin(), ws.Modules.end());
    members.insert(members.end(), ws.Apps.begin(), ws.Apps.end());
    std::ranges::sort(members);
    return members;
}

static auto IsTargetFile(const ShaderFile& file, const std::vector<std::string>& projects, const ShaderGenOptions& options) -> bool {
    if (std::ranges::find(projects, file.OwningProject) == projects.end()) {
        return false;
    }
    if (!options.File) {
        return true;
    }
    auto ec = std::error_code();
    return std::filesystem::equivalent(file.Path, *options.File, ec);
}

static auto RunShaderGen(const Workspace& ws, const ShaderGenOptions& options, const ShaderCatalog& catalog) -> std::expected<void, std::string> {
    const auto projects = TargetProjects(ws, options);
    if (!projects) {
        return std::unexpected(projects.error());
    }

    auto scopes = std::unordered_map<std::string, SchemeScope>();
    auto stale = std::vector<std::filesystem::path>();
    auto generated = 0;

    for (const auto& file : catalog.Files) {
        if (!IsTargetFile(file, *projects, options)) continue;

        auto scope = scopes.find(file.OwningProject);
        if (scope == scopes.end()) {
            const auto built = ScopeForProject(ws, catalog, file.OwningProject);
            if (!built) {
                return std::unexpected(built.error());
            }
            scope = scopes.emplace(file.OwningProject, std::move(*built)).first;
        }

        const auto result = GenerateShaderSource(file, scope->second);
        if (!result) {
            return std::unexpected(std::format("{}: {}", file.Path.string(), result.error()));
        }
        if (result->Skipped) continue;
        if (result->NewSource == file.Source) continue;

        if (options.CheckOnly) {
            stale.push_back(file.Path);
            continue;
        }

        auto out = std::ofstream(file.Path);
        out << result->NewSource;
        if (out.fail()) {
            return std::unexpected(std::format("failed to write {}", file.Path.string()));
        }
        std::println("bechef: regenerated {}", file.Path.filename().string());
        generated++;
    }

    if (!stale.empty()) {
        auto joined = std::string();
        for (const auto& path : stale) joined += "\n  " + path.string();
        return std::unexpected(std::format("{} shader(s) have a stale @be-auto-boilerplate region, run 'bechef shadergen':{}", stale.size(), joined));
    }

    if (!options.CheckOnly && generated == 0) {
        std::println("bechef: all shader boilerplate up to date");
    }
    return {};
}

static auto WaitForChange(const std::vector<std::filesystem::path>& watched) -> void {
    auto timestamps = std::unordered_map<std::string, std::filesystem::file_time_type>();
    for (const auto& path : watched) {
        auto ec = std::error_code();
        timestamps[path.string()] = std::filesystem::last_write_time(path, ec);
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (timestamps.empty()) {
            return;
        }

        for (const auto& [path, stamp] : timestamps) {
            auto ec = std::error_code();
            if (std::filesystem::last_write_time(path, ec) != stamp) {
                return;
            }
        }
    }
}

auto ShaderGen(const Workspace& ws, const ShaderGenOptions& options) -> std::expected<void, std::string> {
    if (!options.Watch) {
        const auto catalog = BuildShaderCatalog(ws);
        if (!catalog) {
            return std::unexpected(catalog.error());
        }
        return RunShaderGen(ws, options, *catalog);
    }

    std::println("bechef: watching shaders (Ctrl+C to stop)...");
    auto watched = std::vector<std::filesystem::path>();

    while (true) {
        const auto catalog = BuildShaderCatalog(ws);
        if (!catalog) {
            std::println(stderr, "bechef: {}", catalog.error());
        }
        else {
            const auto result = RunShaderGen(ws, options, *catalog);
            if (!result) {
                std::println(stderr, "bechef: {}", result.error());
            }

            watched.clear();
            for (const auto& file : catalog->Files) {
                watched.push_back(file.Path);
            }
        }

        WaitForChange(watched);
    }
}
