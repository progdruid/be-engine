#include "ShaderGen.h"

#include <print>
#include <format>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <system_error>
#include <thread>
#include <chrono>

#include "Load.h"
#include "Workspace.h"
#include "Verify.h"
#include "ShaderBoilerplate.h"
#include "Try.h"

static auto DetermineTargetProjects(const ShaderGenOptions& options) -> std::expected<std::vector<const Project*>, std::string> {
    const auto& workspace = Workspace::Get();

    if (options.File) {
        bechef_try(const auto* owner, workspace.FindOwningProject(*options.File));
        return std::vector<const Project*>{ owner };
    }

    if (options.Project) {
        const auto* entry = workspace.FindProject(*options.Project);
        if (!entry) {
            return std::unexpected(std::format("'{}' is not a project in workspace.bechef", *options.Project));
        }
        if (!*entry) {
            return std::unexpected(entry->error());
        }
        return std::vector<const Project*>{ &**entry };
    }

    return workspace.AllProjects();
}

static auto RunShaderGen(const ShaderGenOptions& options) -> std::expected<void, std::string> {
    bechef_try(const auto projects, DetermineTargetProjects(options));

    for (const auto* project : projects) {
        const auto verified = VerifyProject(*project);
        if (!verified) {
            return std::unexpected(verified.error());
        }
    }

    auto stale = std::vector<std::filesystem::path>();
    auto generated = 0;

    for (const auto* project : projects) {
        for (const auto& shader : project->LocalShaders) {
            auto ec = std::error_code();
            if (options.File && !std::filesystem::equivalent(shader.Path, *options.File, ec)) {
                continue;
            }

            const auto newSource = GenerateShaderSource(shader);
            if (!newSource) continue;
            if (*newSource == shader.Data->Source) continue;

            if (options.CheckOnly) {
                stale.push_back(shader.Path);
                continue;
            }

            auto out = std::ofstream(shader.Path);
            out << *newSource;
            if (out.fail()) {
                return std::unexpected(std::format("failed to write {}", shader.Path.string()));
            }
            std::println("bechef: regenerated {}", shader.Path.filename().string());
            generated++;
        }
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

static auto WaitForChange() -> void {
    auto timestamps = std::unordered_map<std::string, std::filesystem::file_time_type>();
    for (const auto* project : Workspace::Get().AllProjects()) {
        for (const auto& shader : project->LocalShaders) {
            auto ec = std::error_code();
            timestamps[shader.Path.string()] = std::filesystem::last_write_time(shader.Path, ec);
        }
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

auto ShaderGen(const ShaderGenOptions& options) -> std::expected<void, std::string> {
    if (!options.Watch) {
        return RunShaderGen(options);
    }

    std::println("bechef: watching shaders (Ctrl+C to stop)...");

    while (true) {
        const auto result = RunShaderGen(options);
        if (!result) {
            std::println(stderr, "bechef: {}", result.error());
        }

        WaitForChange();

        const auto reloaded = ReloadWorkspace();
        if (!reloaded) {
            std::println(stderr, "bechef: {}", reloaded.error());
            return reloaded;
        }
    }
}
