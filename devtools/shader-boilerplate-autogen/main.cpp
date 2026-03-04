#include <print>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <expected>

#include "BeShaderTools.h"
#include "umbrellas/include-libassert.h"

static const char* RegionBegin = "// region @be-auto-boilerplate";
static const char* RegionEnd   = "// endregion";

struct TargetEntry {
    std::string Name;
    std::string Type;
    uint32_t    Slot;
};

static auto VertexFieldForLayout(const std::string& layout) -> std::string {
    if (layout == "position") return "float3 Position : POSITION;";
    if (layout == "normal")   return "float3 Normal : NORMAL;";
    if (layout == "color3")   return "float3 Color : COLOR;";
    if (layout == "color4")   return "float4 Color : COLOR;";
    if (layout == "uv0")      return "float2 UV : TEXCOORD0;";
    if (layout == "uv1")      return "float2 UV1 : TEXCOORD1;";
    if (layout == "uv2")      return "float2 UV2 : TEXCOORD2;";
    return "// unknown layout: " + layout;
}

static auto GenerateBoilerplate(const Json& meta) -> std::string {
    auto out = std::string();

    if (meta.contains("vertexLayout")) {
        out += "struct VertexInput {\n";
        for (const auto& item : meta["vertexLayout"]) {
            out += "    " + VertexFieldForLayout(item.get<std::string>()) + "\n";
        }
        out += "};\n";
    }

    if (meta.contains("targets")) {
        auto entries = std::vector<TargetEntry>();
        for (const auto& [name, val] : meta["targets"].items()) {
            entries.push_back({ name, val["type"].get<std::string>(), val["slot"].get<uint32_t>() });
        }
        std::ranges::sort(entries, [](const TargetEntry& a, const TargetEntry& b) {
            return a.Slot < b.Slot;
        });

        if (!entries.empty()) {
            if (meta.contains("vertexLayout")) out += "\n";
            out += "struct PixelOutput {\n";
            for (const auto& e : entries) {
                out += "    " + e.Type + " " + e.Name + " : SV_Target" + std::to_string(e.Slot) + ";\n";
            }
            out += "};";
        }
    }

    return out;
}

static auto WriteToRegion(const std::string& src, size_t regionBegin, size_t regionEnd,
                           const std::filesystem::path& path, const std::string& content) -> void {
    auto insertStart = src.find('\n', regionBegin);
    if (insertStart == std::string::npos) insertStart = regionBegin + std::strlen(RegionBegin);
    else insertStart++;

    be_assert(insertStart <= regionEnd, "region begin marker must precede endregion on a separate line");

    auto newSrc = src.substr(0, insertStart) + content + src.substr(regionEnd);
    auto file = std::ofstream(path);
    file << newSrc;
    if (file.fail())
        std::println(stderr, "Failed to write {}", path.filename().string());
}

static auto TryProcessFile(const std::filesystem::path& path) -> void {
    auto src = BeShaderTools::ReadFile(path);

    auto regionBegin = src.find(RegionBegin);
    if (regionBegin == std::string::npos)
        return;

    auto regionEnd = src.find(RegionEnd, regionBegin);
    if (regionEnd == std::string::npos)
        return;

    auto writeError = [&](const std::string& msg) {
        WriteToRegion(src, regionBegin, regionEnd, path, "\n// ERROR: " + msg + "\n\n");
        std::println(stderr, "Error: {}", msg);
    };

    Json meta;
    try {
        auto [parsedMeta, shaderName] = BeShaderTools::ParseFor(src, "@be-shader:");
        if (parsedMeta.empty()) {
            writeError("No @be-shader: block found");
            return;
        }
        meta = std::move(parsedMeta);
    } catch (const std::exception& e) {
        writeError(std::string("Failed to parse shader metadata: ") + e.what());
        return;
    }

    WriteToRegion(src, regionBegin, regionEnd, path, "\n" + GenerateBoilerplate(meta) + "\n\n");
    std::println("Updated {}", path.filename().string());
}

void main(int argc, char* argv[]) {
    if (argc < 3 || (std::string(argv[1]) != "--once" && std::string(argv[1]) != "--watch")) {
        std::println(stderr, "Usage:");
        std::println(stderr, "  shader-boilerplate-autogen --once  <shader.hlsl>");
        std::println(stderr, "  shader-boilerplate-autogen --watch <shader.hlsl>");
        return;
    }

    auto mode = std::string(argv[1]);
    auto path = std::filesystem::path(argv[2]);

    if (mode == "--once") {
        TryProcessFile(path);
        return;
    }

    // --watch: poll last_write_time, re-run on change
    std::println("Watching {} (Ctrl+C to stop)...", path.filename().string());

    auto lastWriteTime = std::filesystem::file_time_type{};

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (!std::filesystem::exists(path)) continue;

        auto currentWriteTime = std::filesystem::last_write_time(path);
        std::println(stdout, "Write Time: {}", currentWriteTime);
        if (currentWriteTime == lastWriteTime) 
            continue;

        TryProcessFile(path);
        
        // Update after our own write so we don't re-trigger on our own output
        lastWriteTime = std::filesystem::last_write_time(path);
    }
}
