#include <print>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <chrono>
#include <expected>

#include "BeShaderTools.h"
#include "umbrellas/include-libassert.h"

static const char* RegionBegin       = "// region @be-auto-boilerplate";
static const char* RegionEnd         = "// endregion";
static const char* SingleLineTrigger = "@be-auto-boilerplate";
static const char* Decorator         = "/*========================================================*/";

// -------------------------------------------------------------------------

struct TargetEntry {
    std::string Name;
    std::string Type;
    uint32_t    Slot;
};

using MaterialBlock = BeShaderTools::ParsedMaterial;

// -------------------------------------------------------------------------

static auto KebabToSnakeCase(const std::string& s) -> std::string {
    auto result = s;
    std::ranges::replace(result, '-', '_');
    return result;
}

static auto KebabToPascalCase(const std::string& s) -> std::string {
    auto result = std::string();
    auto cap = true;
    for (unsigned char c : s) {
        if (c == '-') { cap = true; continue; }
        result += cap ? (char)std::toupper(c) : (char)c;
        cap = false;
    }
    return result;
}

struct SchemeEntry {
    std::filesystem::path Path;
    MaterialBlock         Block;
};
using SchemeRegistry = std::unordered_map<std::string, SchemeEntry>;

static auto ParseAllMaterials(const std::string& src) -> std::vector<MaterialBlock> {
    auto materials = BeShaderTools::ParseMaterials(src);
    if (!materials) {
        return {};
    }
    return std::move(*materials);
}

static auto BuildRegistry(const std::vector<std::filesystem::path>& files) -> SchemeRegistry {
    auto registry = SchemeRegistry();
    for (const auto& p : files) {
        if (!std::filesystem::exists(p)) continue;
        try {
            for (auto& mat : ParseAllMaterials(BeShaderTools::ReadFile(p)))
                registry[mat.Name] = { p, mat };
        } catch (...) {}
    }
    return registry;
}

// -------------------------------------------------------------------------


static auto VertexFieldForLayout(const std::string& layout) -> std::string {
    if (layout == "position") return "float3 Position : POSITION;";
    if (layout == "normal")   return "float3 Normal : NORMAL;";
    if (layout == "color3")   return "float3 Color : COLOR;";
    if (layout == "color4")   return "float4 Color : COLOR;";
    if (layout == "uv0")      return "float2 UV : TEXCOORD0;";
    if (layout == "tangent")  return "float4 Tangent : TANGENT;";
    return "// unknown layout: " + layout;
}

static auto IsTextureOrSampler(const std::string& type) -> bool {
    return type == "texture2d" || type == "textureCube" || type == "storage texture2d" || type == "sampler";
}

static auto GenerateMaterialBlock(const MaterialBlock& mat) -> std::string {
    auto structFields = std::string();

    for (const auto& prop : mat.Properties) {
        if (IsTextureOrSampler(prop.Type)) continue;
        auto hlslType = (prop.Type == "matrix") ? std::string("float4x4") : prop.Type;
        structFields += "    " + hlslType + " " + prop.Name + ";\n";
    }

    if (structFields.empty()) return "";
    return "struct " + KebabToSnakeCase(mat.Name) + " {\n" + structFields + "};";
}

static auto GenerateBoilerplate(const BeShaderTools::ParsedShader& shader, const std::vector<MaterialBlock>& materials,
                                const SchemeRegistry& registry, const std::filesystem::path& shaderPath) -> std::string {
    auto parts = std::vector<std::string>();

    // Includes for external schemes referenced in binds
    if (!shader.Binds.empty()) {
        auto includes = std::string();
        auto seen     = std::vector<std::filesystem::path>();
        for (const auto& bind : shader.Binds) {
            auto it = registry.find(bind.Scheme);
            if (it == registry.end()) continue;
            if (std::filesystem::equivalent(it->second.Path, shaderPath)) continue;
            auto rel = std::filesystem::relative(it->second.Path, shaderPath.parent_path());
            if (std::ranges::find(seen, rel) != seen.end()) continue;
            seen.push_back(rel);
            includes += "#include \"" + rel.generic_string() + "\"\n";
        }
        if (!includes.empty()) {
            includes.pop_back();
            parts.push_back(includes);
        }
    }

    // Inline material structs
    for (const auto& mat : materials) {
        auto s = GenerateMaterialBlock(mat);
        if (!s.empty()) parts.push_back(s);
    }

    // Per-material bindings: cbuffer + samplers + textures with register spaces
    if (!shader.Binds.empty()) {
        struct BindingsEntry { uint32_t Slot; std::string Code; };
        auto entries = std::vector<BindingsEntry>();

        for (const auto& bind : shader.Binds) {
            auto schemeName = bind.Scheme;
            auto slot       = uint32_t(bind.Slot);
            auto space      = std::to_string(slot);

            auto* block = [&]() -> const MaterialBlock* {
                for (const auto& mat : materials)
                    if (mat.Name == schemeName) return &mat;
                if (auto it = registry.find(schemeName); it != registry.end()) return &it->second.Block;
                return nullptr;
            }();
            if (!block) continue;

            struct PropEntry { std::string Name; std::string Type; };
            auto scalars  = std::vector<PropEntry>();
            auto samplers = std::vector<PropEntry>();
            auto textures = std::vector<PropEntry>();

            for (const auto& prop : block->Properties) {
                if (prop.Type == "sampler")
                    samplers.push_back({ prop.Name, prop.Type });
                else if (prop.Type == "texture2d" || prop.Type == "textureCube" || prop.Type == "storage texture2d")
                    textures.push_back({ prop.Name, prop.Type });
                else
                    scalars.push_back({ prop.Name, prop.Type });
            }

            auto code = std::string();

            if (!scalars.empty()) {
                auto structName = KebabToSnakeCase(schemeName);
                auto varName    = bind.Var.empty() ? KebabToPascalCase(bind.Link) : bind.Var;
                code += "cbuffer CBuffer_" + space + " : register(b0, space" + space + ") {\n"
                     +  "    " + structName + " _" + varName + ";\n"
                     +  "};";
            }

            for (size_t i = 0; i < samplers.size(); i++) {
                if (!code.empty()) code += "\n";
                code += "SamplerState " + samplers[i].Name
                     +  " : register(s" + std::to_string(1 + i) + ", space" + space + ");";
            }

            for (size_t i = 0; i < textures.size(); i++) {
                if (!code.empty()) code += "\n";
                auto reg = std::to_string(1 + samplers.size() + i);

                auto hlslType  = std::string("Texture2D");
                auto regLetter = std::string("t");
                if (textures[i].Type == "storage texture2d") {
                    hlslType  = "RWTexture2D<float4>";
                    regLetter = "u";
                } else if (textures[i].Type == "textureCube") {
                    hlslType  = "TextureCube";
                }

                code += hlslType + " " + textures[i].Name
                     +  " : register(" + regLetter + reg + ", space" + space + ");";
            }

            if (!code.empty())
                entries.push_back({ slot, std::move(code) });
        }

        std::ranges::sort(entries, {}, &BindingsEntry::Slot);
        if (!entries.empty()) {
            auto s = std::string();
            for (size_t i = 0; i < entries.size(); i++) {
                if (i > 0) s += "\n\n";
                s += entries[i].Code;
            }
            parts.push_back(s);
        }
    }

    // VertexInput / PixelOutput
    if (!shader.VertexLayout.empty()) {
        auto s = std::string("struct VertexInput {\n");
        for (const auto& item : shader.VertexLayout)
            s += "    " + VertexFieldForLayout(item) + "\n";
        s += "};";
        parts.push_back(s);
    }

    if (!shader.Targets.empty()) {
        auto entries = std::vector<TargetEntry>();
        for (const auto& target : shader.Targets)
            entries.push_back({ target.Name, target.Type, uint32_t(target.Slot) });
        std::ranges::sort(entries, {}, &TargetEntry::Slot);

        if (!entries.empty()) {
            auto s = std::string("struct PixelOutput {\n");
            for (const auto& e : entries)
                s += "    " + e.Type + " " + e.Name + " : SV_Target" + std::to_string(e.Slot) + ";\n";
            s += "};";
            parts.push_back(s);
        }
    }

    auto out = std::string();
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) out += "\n\n";
        out += parts[i];
    }
    return out;
}

// -------------------------------------------------------------------------

static auto WriteToRegion(const std::string& src, size_t regionBegin, size_t regionEnd,
                           const std::filesystem::path& path, const std::string& content) -> void {
    // Expand start back over preceding decorator line if present
    auto blockStart = regionBegin;
    if (regionBegin > 0 && src[regionBegin - 1] == '\n') {
        auto prevLineStart = (regionBegin < 2) ? size_t(0) : src.rfind('\n', regionBegin - 2);
        prevLineStart = (prevLineStart == std::string::npos) ? 0 : prevLineStart + 1;
        if (src.compare(prevLineStart, std::strlen(Decorator), Decorator) == 0)
            blockStart = prevLineStart;
    }

    // Find end of endregion line, expand over following decorator line if present
    auto blockEnd = src.find('\n', regionEnd);
    if (blockEnd == std::string::npos) blockEnd = src.size();
    else blockEnd++;
    if (blockEnd < src.size() && src.compare(blockEnd, std::strlen(Decorator), Decorator) == 0) {
        auto nextNl = src.find('\n', blockEnd);
        blockEnd = (nextNl == std::string::npos) ? src.size() : nextNl + 1;
    }

    auto newBlock = std::string(Decorator) + "\n"
                  + RegionBegin + "\n"
                  + content
                  + RegionEnd + "\n"
                  + Decorator + "\n";
    auto newSrc = src.substr(0, blockStart) + newBlock + src.substr(blockEnd);
    auto file = std::ofstream(path);
    file << newSrc;
    if (file.fail())
        std::println(stderr, "Failed to write {}", path.filename().string());
}

static auto LineStart(const std::string& src, size_t pos) -> size_t {
    auto ls = src.rfind('\n', pos);
    return (ls == std::string::npos) ? 0 : ls + 1;
}

static auto TryProcessFile(const std::filesystem::path& path, const SchemeRegistry& registry) -> void {
    auto src = BeShaderTools::ReadFile(path);

    auto regionBegin = src.find(RegionBegin);
    auto regionEnd   = src.find(RegionEnd, regionBegin);

    if (regionBegin == std::string::npos || regionEnd == std::string::npos) {
        // No region block — find trigger and expand it in place
        auto triggerPos = src.find(SingleLineTrigger);
        if (triggerPos == std::string::npos) return;
        auto ls = LineStart(src, triggerPos);
        auto le = src.find('\n', ls);
        if (le == std::string::npos) le = src.size(); else le++;
        src = src.substr(0, ls) + Decorator + "\n" + RegionBegin + "\n\n" + RegionEnd + "\n" + Decorator + "\n" + src.substr(le);
        regionBegin = ls + std::strlen(Decorator) + 1; // skip decorator line
        regionEnd   = src.find(RegionEnd, regionBegin);
    } else {
        // Region exists — delete stray trigger if present (skip the region begin line itself)
        auto triggerPos = src.find(SingleLineTrigger);
        while (triggerPos != std::string::npos) {
            auto ls = LineStart(src, triggerPos);
            if (src.compare(ls, std::strlen(RegionBegin), RegionBegin) != 0) {
                auto le = src.find('\n', ls);
                if (le == std::string::npos) le = src.size(); else le++;
                src.erase(ls, le - ls);
                regionBegin = src.find(RegionBegin);
                regionEnd   = src.find(RegionEnd, regionBegin);
                break;
            }
            triggerPos = src.find(SingleLineTrigger, triggerPos + 1);
        }
    }

    auto writeError = [&](const std::string& msg) {
        WriteToRegion(src, regionBegin, regionEnd, path, "// ERROR: " + msg + "\n\n");
        std::println(stderr, "Error: {}", msg);
    };

    // Material-only files (includes) have no @be-shader block; that is not an error.
    auto shader = BeShaderTools::ParsedShader();
    if (src.find("@be-shader") != std::string::npos) {
        auto parsed = BeShaderTools::ParseShader(src);
        if (!parsed) {
            writeError("Failed to parse @be-shader -> " + parsed.error());
            return;
        }
        shader = std::move(*parsed);
    }

    auto materials = ParseAllMaterials(src);

    WriteToRegion(src, regionBegin, regionEnd, path, GenerateBoilerplate(shader, materials, registry, path) + "\n\n");
    std::println("Updated {}", path.filename().string());
}

// -------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 3 || (std::string(argv[1]) != "--once" && std::string(argv[1]) != "--watch")) {
        std::println(stderr, "Usage:");
        std::println(stderr, "  shader-boilerplate-autogen --once  <shader.hlsl>");
        std::println(stderr, "  shader-boilerplate-autogen --watch <shader.hlsl> [<shader2.hlsl> ...]");
        return 1;
    }

    auto mode = std::string(argv[1]);

    if (mode == "--once") {
        auto shaderPath = std::filesystem::path(argv[2]);
        auto files = std::vector<std::filesystem::path>();
        for (auto& e : std::filesystem::recursive_directory_iterator(shaderPath.parent_path()))
            if (e.path().extension() == ".hlsl") files.push_back(e.path());
        TryProcessFile(shaderPath, BuildRegistry(files));
        return 0;
    }

    // --watch: collect all paths, poll last_write_time for each
    // Arguments may be individual .hlsl files or directories (recursively expanded)
    auto watchedFiles = std::unordered_map<std::filesystem::path, std::filesystem::file_time_type>();
    for (int i = 2; i < argc; i++) {
        auto entry = std::filesystem::path(argv[i]);
        if (!std::filesystem::is_directory(entry)) {
            watchedFiles[entry] = {};
            std::println("Found file: {}", entry.string());
            continue;
        }
        for (auto& dirEntry : std::filesystem::recursive_directory_iterator(entry)) {
            if (dirEntry.path().extension() == ".hlsl") {
                watchedFiles[dirEntry.path()] = {};
                std::println("Found file: {}", dirEntry.path().string());
            }
        }
    }

    std::println("Watching {} file(s) (Ctrl+C to stop)...", watchedFiles.size());

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        for (auto& [path, lastWriteTime] : watchedFiles) {
            if (!std::filesystem::exists(path)) continue;

            auto currentWriteTime = std::filesystem::last_write_time(path);
            if (currentWriteTime == lastWriteTime) continue;

            auto allFiles = std::vector<std::filesystem::path>();
            for (const auto& [p, _] : watchedFiles) allFiles.push_back(p);
            TryProcessFile(path, BuildRegistry(allFiles));

            // Update after our own write so we don't re-trigger on our own output
            lastWriteTime = std::filesystem::last_write_time(path);
        }
    }
}
