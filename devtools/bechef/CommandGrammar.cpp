#include "CommandGrammar.h"

#include <format>
#include <algorithm>

#include "BeShaderTools.h"

auto ParsedCommand::Get(std::string_view key) const -> std::string {
    const auto it = Flags.find(std::string(key));
    return it == Flags.end() ? "" : it->second;
}

auto ParsedCommand::Has(std::string_view key) const -> bool {
    return Flags.contains(std::string(key));
}

auto CommandGrammar::Parse(const RawCommand& command) const -> std::expected<ParsedCommand, std::string> {
    const auto rule = std::ranges::find(Rules, command.Verb, &VerbRule::Verb);
    if (rule == Rules.end()) {
        return std::unexpected(std::format("unknown command '{}'", command.Verb));
    }

    auto out = ParsedCommand();
    for (size_t i = 0; i < command.Args.size(); i++) {
        const auto arg = std::string_view(command.Args[i]);
        if (arg.starts_with("--")) {
            const auto name = std::string(arg.substr(2));
            const auto flag = std::ranges::find(rule->Flags, name, &FlagRule::Flag);
            if (flag == rule->Flags.end()) {
                return std::unexpected(std::format("{}: unknown flag '--{}'", rule->Verb, name));
            }
            if (!flag->TakesValue) {
                out.Flags[name] = "true";
                continue;
            }
            if (i + 1 >= command.Args.size()) {
                return std::unexpected(std::format("{}: '--{}' needs a value", rule->Verb, name));
            }
            out.Flags[name] = command.Args[++i];
        }
        else if (rule->AllowPositionals) {
            out.Positionals.push_back(std::string(arg));
        }
        else {
            return std::unexpected(std::format("{}: unexpected '{}'", rule->Verb, arg));
        }
    }
    for (const auto& flag : rule->Flags) {
        if (flag.Required && !out.Has(flag.Flag)) {
            return std::unexpected(std::format("{}: '--{}' is required", rule->Verb, flag.Flag));
        }
    }
    return out;
}

auto ParseRawCommands(std::string_view text) -> std::vector<RawCommand> {
    auto out = std::vector<RawCommand>();
    for (const auto rawLine : BeShaderTools::Split(text, "\n")) {
        const auto line = BeShaderTools::Trim(rawLine.substr(0, rawLine.find('#')), " \t\r");
        if (line.empty()) continue;

        const auto tokens = BeShaderTools::Split(line, " \t");
        auto args = std::vector<std::string>(tokens.begin() + 1, tokens.end());
        out.push_back({ std::string(tokens[0]), std::move(args) });
    }
    return out;
}
