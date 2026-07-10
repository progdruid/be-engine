#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <expected>

struct RawCommand {
    std::string Verb;
    std::vector<std::string> Args;
};

struct FlagRule {
    std::string Flag;
    bool TakesValue = true;
    bool Required = false;
};

struct VerbRule {
    std::string Verb;
    std::vector<FlagRule> Flags;
    bool AllowPositionals = false;
};

struct ParsedCommand {
    std::unordered_map<std::string, std::string> Flags;
    std::vector<std::string> Positionals;

    auto Get(std::string_view key) const -> std::string;
    auto Has(std::string_view key) const -> bool;
};

struct CommandGrammar {
    std::vector<VerbRule> Rules;

    auto Parse(const RawCommand& command) const -> std::expected<ParsedCommand, std::string>;
};

auto ParseRawCommands(std::string_view text) -> std::vector<RawCommand>;
