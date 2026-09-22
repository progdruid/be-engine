#pragma once
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <common.hpp>

class BeShaderTools {
    expose
    static auto ReadFile (const std::filesystem::path& path) -> std::string;

    struct Block {
        std::string Name;
        std::vector<std::string> Lines;
        size_t End = 0;
    };

    struct SourceBlocks {
        std::optional<Block> Shader;
        std::vector<Block>   Materials;
    };
    static auto FindBlocks (const std::string& src) -> SourceBlocks;

    struct ParsedMaterialProperty {
        std::string Name;
        std::string Type;
        uint32_t    ArrayLength = 1;
        std::string Default;
    };
    static auto ParseMaterialProperty (const std::string& text) -> std::expected<ParsedMaterialProperty, std::string>;

    struct ParsedMaterial {
        std::string Name;
        std::vector<ParsedMaterialProperty> Properties;
        std::filesystem::path SourceFile;
    };
    static auto ParseMaterialBlock (const Block& block) -> std::expected<ParsedMaterial, std::string>;

    static auto IsSampler (const std::string& type) -> bool;
    static auto IsTexture (const std::string& type) -> bool;

    struct ParsedBind {
        std::string Link;
        std::string Scheme;
        uint8_t     Slot = 0;
        std::string Var;
    };
    struct ParsedTarget {
        std::string Name;
        std::string Type;
        uint8_t     Slot = 0;
    };

    enum class RootFieldKind { Pointer, TextureIndex, SamplerIndex };
    struct RootField {
        RootFieldKind Kind;
        std::string   Link;
        std::string   FieldName;
        std::string   AliasName;
        std::string   TypeName;
        std::string   TableArray;
        std::string   PropertyName;
        uint32_t      Offset = 0;
    };
    struct RootLayout {
        std::vector<RootField> Fields;
        uint32_t Size = 0;
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
        std::filesystem::path SourceFile;
        std::optional<RootLayout> Root;
    };
    static auto ParseShaderBlock (const Block& block) -> std::expected<ParsedShader, std::string>;

    struct ParsedShaderFile {
        std::optional<ParsedShader> Shader;
        std::vector<ParsedMaterial> Materials;
    };
    static auto ParseShaderFile (const std::string& src, const std::filesystem::path& path) -> std::expected<ParsedShaderFile, std::string>;

    static auto BuildRootLayout (const ParsedShader& shader, const std::vector<ParsedMaterial>& materials) -> RootLayout;
    static auto SchemeStructName (const std::string& schemeName) -> std::string;

    static auto ParseFloat (const std::string& text) -> std::expected<float, std::string>;
    static auto ParseTuple (const std::string& text) -> std::expected<std::vector<float>, std::string>;
    static auto ParseFloatArray (const std::string& text, uint32_t componentCount) -> std::expected<std::vector<float>, std::string>;

    static auto Take (std::string_view str, size_t start, size_t end) -> std::string_view;
    static auto Trim (std::string_view str, const char* trimmedChars) -> std::string_view;
    static auto Split (std::string_view str, const char* delimiters) -> std::vector<std::string_view>;
};
