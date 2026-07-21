#include "ShaderBoilerplate.h"

#include <vector>
#include <optional>
#include <algorithm>

static const char* RegionBegin = "// region @be-auto-boilerplate";
static const char* RegionEnd = "// endregion";
static const char* SingleLineTrigger = "@be-auto-boilerplate";
static const char* Decorator = "/*========================================================*/";

static auto SplitLines(const std::string& text) -> std::vector<std::string> {
    auto lines = std::vector<std::string>();
    auto start = size_t(0);
    while (true) {
        const auto end = text.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(text.substr(start));
            return lines;
        }
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
}

static auto JoinLines(const std::vector<std::string>& lines) -> std::string {
    auto text = std::string();
    for (size_t i = 0; i < lines.size(); i++) {
        if (i > 0) text += "\n";
        text += lines[i];
    }
    return text;
}

static auto LineAt(const std::vector<std::string>& lines, size_t at) -> std::string_view {
    if (at >= lines.size()) return {};
    return BeShaderTools::Trim(lines[at], " \t\r");
}

static auto FindLine(const std::vector<std::string>& lines, std::string_view text, size_t from = 0) -> std::optional<size_t> {
    for (auto i = from; i < lines.size(); i++) {
        if (LineAt(lines, i) == text) return i;
    }
    return std::nullopt;
}

static auto KebabToSnakeCase(const std::string& text) -> std::string {
    auto result = text;
    std::ranges::replace(result, '-', '_');
    return result;
}

static auto KebabToPascalCase(const std::string& text) -> std::string {
    auto result = std::string();
    auto capitalise = true;
    for (const unsigned char c : text) {
        if (c == '-') {
            capitalise = true;
            continue;
        }
        result += capitalise ? char(std::toupper(c)) : char(c);
        capitalise = false;
    }
    return result;
}

static auto VertexFieldForLayout(const std::string& layout) -> std::string {
    if (layout == "position") return "float3 Position : POSITION;";
    if (layout == "normal") return "float3 Normal : NORMAL;";
    if (layout == "color3") return "float3 Color : COLOR;";
    if (layout == "color4") return "float4 Color : COLOR;";
    if (layout == "uv0") return "float2 UV : TEXCOORD0;";
    if (layout == "tangent") return "float4 Tangent : TANGENT;";
    return "// unknown layout: " + layout;
}

static auto IsSampler(const std::string& type) -> bool {
    return type == "sampler";
}

static auto IsTexture(const std::string& type) -> bool {
    return type == "texture2d" || type == "textureCube" || type == "storage texture2d";
}

static auto GenerateIncludes(const std::filesystem::path& path, const std::vector<ResolvedBind>& binds) -> std::vector<std::string> {
    const auto ownFile = path.filename().string();

    auto includes = std::vector<std::string>();
    for (const auto& bind : binds) {
        const auto declaredIn = bind.Scheme->File.filename().string();
        if (declaredIn == ownFile) continue;

        auto include = "#include \"" + declaredIn + "\"";
        if (std::ranges::find(includes, include) != includes.end()) continue;
        includes.push_back(std::move(include));
    }
    return includes;
}

static auto GenerateMaterialStruct(const BeShaderTools::ParsedMaterial& material) -> std::optional<std::string> {
    auto fields = std::string();
    for (const auto& property : material.Properties) {
        if (IsSampler(property.Type) || IsTexture(property.Type)) continue;
        const auto type = property.Type == "matrix" ? std::string("float4x4") : property.Type;
        const auto suffix = property.ArrayLength > 1 ? "[" + std::to_string(property.ArrayLength) + "]" : std::string();
        fields += "    " + type + " " + property.Name + suffix + ";\n";
    }

    if (fields.empty()) {
        return std::nullopt;
    }
    return "struct " + KebabToSnakeCase(material.Name) + " {\n" + fields + "};";
}

static auto GenerateBindings(const ResolvedBind& bind) -> std::optional<std::string> {
    const auto space = std::to_string(bind.Bind.Slot);
    const auto& scheme = bind.Scheme->Material;

    auto scalars = std::vector<BeShaderTools::ParsedMaterialProperty>();
    auto samplers = std::vector<BeShaderTools::ParsedMaterialProperty>();
    auto textures = std::vector<BeShaderTools::ParsedMaterialProperty>();

    for (const auto& property : scheme.Properties) {
        if (IsSampler(property.Type)) samplers.push_back(property);
        else if (IsTexture(property.Type)) textures.push_back(property);
        else scalars.push_back(property);
    }

    auto code = std::string();

    if (!scalars.empty()) {
        const auto structName = KebabToSnakeCase(scheme.Name);
        const auto varName = bind.Bind.Var.empty() ? KebabToPascalCase(bind.Bind.Link) : bind.Bind.Var;
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
        const auto reg = std::to_string(1 + samplers.size() + i);

        auto type = std::string("Texture2D");
        auto letter = std::string("t");
        if (textures[i].Type == "storage texture2d") {
            type = "RWTexture2D<float4>";
            letter = "u";
        }
        else if (textures[i].Type == "textureCube") {
            type = "TextureCube";
        }

        code += type + " " + textures[i].Name
             +  " : register(" + letter + reg + ", space" + space + ");";
    }

    if (code.empty()) {
        return std::nullopt;
    }
    return code;
}

static auto GenerateVertexInput(const std::vector<std::string>& layout) -> std::string {
    auto text = std::string("struct VertexInput {\n");
    for (const auto& item : layout) {
        text += "    " + VertexFieldForLayout(item) + "\n";
    }
    text += "};";
    return text;
}

static auto GeneratePixelOutput(const std::vector<BeShaderTools::ParsedTarget>& shaderTargets) -> std::string {
    auto targets = shaderTargets;
    std::ranges::sort(targets, {}, &BeShaderTools::ParsedTarget::Slot);

    auto text = std::string("struct PixelOutput {\n");
    for (const auto& target : targets) {
        text += "    " + target.Type + " " + target.Name + " : SV_Target" + std::to_string(target.Slot) + ";\n";
    }
    text += "};";
    return text;
}

static auto GenerateBoilerplate(const std::filesystem::path& path, const ShaderData& data, const std::vector<ResolvedBind>& binds) -> std::vector<std::string> {
    auto parts = std::vector<std::string>();

    const auto includes = GenerateIncludes(path, binds);
    if (!includes.empty()) {
        parts.push_back(JoinLines(includes));
    }

    for (const auto& material : data.Materials) {
        auto text = GenerateMaterialStruct(material);
        if (text) {
            parts.push_back(std::move(*text));
        }
    }

    for (const auto& bind : binds) {
        auto text = GenerateBindings(bind);
        if (text) {
            parts.push_back(std::move(*text));
        }
    }

    if (data.Shader && !data.Shader->VertexLayout.empty()) {
        parts.push_back(GenerateVertexInput(data.Shader->VertexLayout));
    }

    if (data.Shader && !data.Shader->Targets.empty()) {
        parts.push_back(GeneratePixelOutput(data.Shader->Targets));
    }

    auto block = std::vector<std::string>{ Decorator, RegionBegin };
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) block.push_back("");
        for (auto& line : SplitLines(parts[i])) {
            block.push_back(std::move(line));
        }
    }
    if (parts.empty()) {
        block.push_back("");
    }
    block.push_back("");
    block.push_back(RegionEnd);
    block.push_back(Decorator);

    return block;
}

struct RegionSpan {
    size_t First = 0;
    size_t Last = 0;
};

static auto FindRegionSpan(const std::vector<std::string>& lines) -> std::optional<RegionSpan> {
    const auto beginLine = FindLine(lines, RegionBegin);
    const auto trigger = FindLine(lines, SingleLineTrigger);

    auto span = RegionSpan();
    if (beginLine) {
        span.First = *beginLine;
        span.Last = FindLine(lines, RegionEnd, *beginLine).value_or(*beginLine);
    }
    else if (trigger) {
        span.First = *trigger;
        span.Last = *trigger;
    }
    else {
        return std::nullopt;
    }

    if (span.First > 0 && LineAt(lines, span.First - 1) == Decorator) {
        span.First--;
    }
    if (LineAt(lines, span.Last + 1) == Decorator) {
        span.Last++;
    }
    return span;
}

auto GenerateShaderSource(const ShaderFile& shader) -> std::optional<std::string> {
    if (!shader.Data || !shader.Binds) {
        return std::nullopt;
    }

    const auto& data = *shader.Data;
    auto lines = SplitLines(data.Source);

    const auto span = FindRegionSpan(lines);
    if (!span) {
        return std::nullopt;
    }

    const auto block = GenerateBoilerplate(shader.Path, data, *shader.Binds);

    lines.erase(lines.begin() + span->First, lines.begin() + span->Last + 1);
    lines.insert(lines.begin() + span->First, block.begin(), block.end());

    return JoinLines(lines);
}
