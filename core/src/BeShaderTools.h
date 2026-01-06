#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/json.h>

class BeShaderTools {
    expose
    static auto ReadFile (const std::filesystem::path& path) -> std::string;

    struct ParsedMaterialProperty {
        std::string Name;
        std::string Type;
        int8_t Slot = -1;
        std::string Default;
    };
    static auto ParseMaterialProperty (const std::string& text) -> ParsedMaterialProperty;
    
    static auto ParseFor(const std::string& src, const std::string& target) -> std::pair<Json, std::string>;
    static auto Take (std::string_view str, size_t start, size_t end) -> std::string_view;
    static auto Trim (std::string_view str, const char* trimmedChars) -> std::string_view;
    static auto Split (std::string_view str, const char* delimiters) -> std::vector<std::string_view>;
};
