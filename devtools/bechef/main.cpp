#include <print>
#include <string>
#include <filesystem>
#include <optional>
#include <expected>

#include "CommandGrammar.h"
#include "Workspace.h"
#include "Deploy.h"
#include "Check.h"
#include "ShaderGen.h"

static auto ErrorPrintUsage() -> void {
    std::println(stderr, "Usage:");
    std::println(stderr, "  bechef deploy    --app <name> --out <dir> [--mode copy|symlink] [--root <dir>] [--depfile <path>]");
    std::println(stderr, "  bechef check     [--root <dir>]");
    std::println(stderr, "  bechef shadergen [--project <name>] [--file <path>] [--check] [--watch] [--root <dir>]");
}

auto main(int argc, char* argv[]) -> int {
    static const auto cliGrammar = CommandGrammar{ .Rules = { 
        VerbRule { 
            .Verb = "deploy", .Flags = {
                FlagRule { .Flag = "app", .Required = true },
                FlagRule { .Flag = "out", .Required = true },
                FlagRule { .Flag = "mode" },
                FlagRule { .Flag = "root" },
                FlagRule { .Flag = "depfile" },
            } 
        },
        VerbRule {
            .Verb = "check", .Flags = { FlagRule{ .Flag = "root" }, }
        },
        VerbRule {
            .Verb = "shadergen", .Flags = {
                FlagRule { .Flag = "project" },
                FlagRule { .Flag = "file" },
                FlagRule { .Flag = "check", .TakesValue = false },
                FlagRule { .Flag = "watch", .TakesValue = false },
                FlagRule { .Flag = "root" },
            }
        },
    }};
    
    if (argc < 2) {
        ErrorPrintUsage();
        return 1;
    }
    const auto command = RawCommand{ argv[1], { argv + 2, argv + argc } };
    const auto parsed = cliGrammar.Parse(command);
    if (!parsed) {
        std::println(stderr, "bechef: {}", parsed.error());
        ErrorPrintUsage();
        return 1;
    }


    auto rootDir = std::filesystem::path();
    if (parsed->Has("root")) {
        rootDir = parsed->Get("root");
    } else if (const auto found = FindRoot(std::filesystem::current_path())) {
        rootDir = *found;
    } else {
        std::println(stderr, "bechef: no workspace.bechef found from {}", std::filesystem::current_path().string());
        return 1;
    }


    const auto workspace = LoadWorkspace(rootDir);
    if (!workspace) {
        std::println(stderr, "bechef: {}", workspace.error());
        return 1;
    }

    const auto mode = parsed->Get("mode") == "symlink" ? Mode::Symlink : Mode::Copy;
    const auto app  = parsed->Get("app");
    const auto out  = std::filesystem::path(parsed->Get("out"));

    auto result = std::expected<void, std::string>();
    if (command.Verb == "deploy") {
        auto depfile = std::optional<std::filesystem::path>();
        if (parsed->Has("depfile")) {
            depfile = parsed->Get("depfile");
        }

        result = Deploy(*workspace, app, out, mode, depfile);
    }
    else if (command.Verb == "shadergen") {
        auto options = ShaderGenOptions();
        if (parsed->Has("project")) {
            options.Project = parsed->Get("project");
        }
        if (parsed->Has("file")) {
            options.File = std::filesystem::path(parsed->Get("file"));
        }
        options.CheckOnly = parsed->Has("check");
        options.Watch = parsed->Has("watch");

        result = ShaderGen(*workspace, options);
    }
    else {
        result = Check(*workspace);
    }

    if (!result) {
        std::println(stderr, "bechef: {}", result.error()); return 1;
    }

    return 0;
}
