#pragma once
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <umbrellas/common.hpp>

class BeShaderTools {
    expose
    static auto ReadFile (const std::filesystem::path& path) -> std::string;

    struct ParsedMaterialProperty {
        std::string Name;
        std::string Type;         // "float".."float4", "matrix", "texture2d", "textureCube", "storage texture2d", "sampler"
        uint32_t    ArrayLength = 1; // element count; a trailing "[N]" on the type is stripped into this
        std::string Default;      // raw default token(s); empty when omitted
    };
    static auto ParseMaterialProperty (const std::string& text) -> std::expected<ParsedMaterialProperty, std::string>;
    // Parses a single "Name: type[ [N] ] = default" property line.

    struct ParsedMaterial {
        std::string Name;
        std::vector<ParsedMaterialProperty> Properties;
    };
    static auto ParseMaterials (const std::string& src) -> std::expected<std::vector<ParsedMaterial>, std::string>;
    // Parses every `@be-material` block found in a source file.

    
    struct ParsedBind {
        std::string Link;
        std::string Scheme;
        uint8_t     Slot = 0;
        std::string Var;   // optional generated-variable-name override; empty = derive from Link
    };
    struct ParsedTarget {
        std::string Name;
        std::string Type;
        uint8_t     Slot = 0;
    };
    struct ParsedShader {
        std::string Name; 
        std::string Topology;
        std::string Rasterizer;
        std::string Blend;
        std::string Depth;
        std::string VertexFn;
        std::vector<std::string> VertexLayout;
        std::string PixelFn;
        std::string ComputeFn;
        std::string HullFn;
        std::string DomainFn;
        std::vector<ParsedBind> Binds;
        std::vector<ParsedTarget> Targets;
    };
    // Parses the single `@be-shader` block found in a source file.
    static auto ParseShader (const std::string& src) -> std::expected<ParsedShader, std::string>;

    
    // A brace-delimited metadata block: `@tag <Name> { <Body> }`
    struct Block {
        std::string Name;
        std::vector<std::string> Lines;
        size_t End = 0;
    };
    static auto FindBlock (const std::string& src, const std::string& tag, size_t from = 0) -> std::optional<Block>;

    static auto ParseFloatList (const std::string& text) -> std::expected<std::vector<float>, std::string>;
    // Parses a single "(a, b, c)" tuple into a flat list.

    static auto ParseFloatArray (const std::string& text, uint32_t componentCount) -> std::expected<std::vector<float>, std::string>;
    // Parses "[a, b, ...]" (componentCount == 1) or "[(a,b,c), (a,b,c), ...]" (componentCount > 1) into a flat list.

    static auto Take (std::string_view str, size_t start, size_t end) -> std::string_view;
    static auto Trim (std::string_view str, const char* trimmedChars) -> std::string_view;
    static auto Split (std::string_view str, const char* delimiters) -> std::vector<std::string_view>;
};
