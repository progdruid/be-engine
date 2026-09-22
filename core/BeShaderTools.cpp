#include "BeShaderTools.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "include-glm.h"
#include "include-libassert.h"

namespace {
    auto ParseSlot(std::string_view tok) -> uint8_t {
        if (!tok.empty() && (tok.front() == 's' || tok.front() == 'S')) {
            tok = tok.substr(1);
        }
        return static_cast<uint8_t>(std::stoi(std::string(tok)));
    }

    auto KebabToSnake(const std::string& text) -> std::string {
        auto result = text;
        std::ranges::replace(result, '-', '_');
        return result;
    }

    auto KebabToPascal(const std::string& text) -> std::string {
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

    auto TextureSlangType(const std::string& type) -> std::string {
        if (type == "storage texture2d") return "RWTexture2D<float4>";
        if (type == "textureCube")       return "TextureCube";
        if (type == "texture2d[]")       return "Texture2DArray";
        if (type == "textureCube[]")     return "TextureCubeArray";
        return "Texture2D";
    }

    auto TextureTableArray(const std::string& type) -> std::string {
        if (type == "storage texture2d") return "RWTex2DTable";
        if (type == "textureCube")       return "TexCubeTable";
        if (type == "texture2d[]")       return "Tex2DArrayTable";
        if (type == "textureCube[]")     return "TexCubeArrayTable";
        return "Tex2DTable";
    }
}

auto BeShaderTools::ReadFile(const std::filesystem::path& path) -> std::string {
    be_assert(std::filesystem::exists(path), "file does not exist", path);

    auto file = std::ifstream(path);
    auto buffer = std::stringstream();
    buffer << file.rdbuf();
    auto src = buffer.str();
    return src;
}

auto BeShaderTools::ParseMaterialProperty(const std::string& text) -> std::expected<ParsedMaterialProperty, std::string> {
    auto result = ParsedMaterialProperty();

    const auto colon = text.find(':');
    if (colon == std::string::npos) {
        return std::unexpected("material property missing ':' -> " + text);
    }

    result.Name = std::string(Trim(std::string_view(text).substr(0, colon), " \t\r\n"));

    auto rest = std::string_view(text).substr(colon + 1);
    const auto eq = rest.find('=');
    if (eq != std::string_view::npos) {
        result.Type = std::string(Trim(rest.substr(0, eq), " \t\r\n"));
        result.Default = std::string(Trim(rest.substr(eq + 1), " \t\r\n"));
    } else {
        result.Type = std::string(Trim(rest, " \t\r\n"));
    }

    if (const auto lb = result.Type.find('['); lb != std::string::npos) {
        const auto rb = result.Type.find(']', lb);
        if (rb == std::string::npos) {
            return std::unexpected("unclosed '[' in type -> " + text);
        }
        const auto inner = Trim(Take(result.Type, lb + 1, rb), " \t");
        const auto base = std::string(Trim(std::string_view(result.Type).substr(0, lb), " \t"));
        if (inner.empty()) {
            result.Type = base + "[]";
        } else {
            try {
                result.ArrayLength = std::stoul(std::string(inner));
            } catch (const std::exception&) {
                return std::unexpected("invalid array length -> " + text);
            }
            if (result.ArrayLength == 0) {
                return std::unexpected("array length must be > 0 -> " + text);
            }
            result.Type = base;
        }
    }

    return result;
}

auto BeShaderTools::ParseMaterialBlock(const Block& block) -> std::expected<ParsedMaterial, std::string> {
    auto material = ParsedMaterial();
    material.Name = block.Name;
    for (const auto& line : block.Lines) {
        auto property = ParseMaterialProperty(line);
        if (!property) {
            return std::unexpected("material '" + material.Name + "' -> " + property.error());
        }
        material.Properties.push_back(std::move(*property));
    }
    return material;
}

auto BeShaderTools::IsSampler(const std::string& type) -> bool {
    return type == "sampler" || type == "comparison sampler";
}

auto BeShaderTools::IsTexture(const std::string& type) -> bool {
    return type == "texture2d" || type == "textureCube" || type == "storage texture2d"
        || type == "texture2d[]" || type == "textureCube[]";
}

auto BeShaderTools::ParseShaderBlock(const Block& block) -> std::expected<ParsedShader, std::string> {
    auto result = ParsedShader();
    result.Name = block.Name;

    for (const auto& line : block.Lines) {
        const auto ws = line.find_first_of(" \t");
        const auto keyword = std::string_view(line).substr(0, ws);
        const auto rest =
            (ws == std::string::npos)
            ? std::string_view()
            : Trim(std::string_view(line).substr(ws), " \t\r");

        if      (keyword == "topology")   { result.Topology   = std::string(rest); }
        else if (keyword == "rasterizer") { result.Rasterizer = std::string(rest); }
        else if (keyword == "blend")      { result.Blend      = std::string(rest); }
        else if (keyword == "depth")      { result.Depth      = std::string(rest); }
        else if (keyword == "pixel")      { result.PixelFn    = std::string(rest); }
        else if (keyword == "compute")    { result.ComputeFn  = std::string(rest); }
        else if (keyword == "hull")       { result.HullFn     = std::string(rest); }
        else if (keyword == "domain")     { result.DomainFn   = std::string(rest); }
        else if (keyword == "vertex") {
            const auto paren = rest.find('(');
            if (paren == std::string_view::npos) {
                result.VertexFn = std::string(rest);
            } else {
                result.VertexFn = std::string(Trim(rest.substr(0, paren), " \t\r"));
                const auto close = rest.find(')', paren);
                const auto inner = rest.substr(paren + 1, (close == std::string_view::npos ? rest.size() : close) - (paren + 1));
                for (const auto semView : Split(inner, ",")) {
                    const auto sem = Trim(semView, " \t\r");
                    if (!sem.empty()) {
                        result.VertexLayout.emplace_back(sem);
                    }
                }
            }
        }
        else if (keyword == "bind") {
            const auto toks = Split(rest, " \t");
            if (toks.size() < 3) {
                return std::unexpected("bind needs 's<slot> <link> <scheme> [<var>]' -> " + line);
            }
            auto bind   = ParsedBind();
            bind.Slot   = ParseSlot(toks[0]);
            bind.Link   = std::string(toks[1]);
            bind.Scheme = std::string(toks[2]);
            if (toks.size() >= 4) {
                bind.Var = std::string(toks[3]);
            }
            result.Binds.push_back(std::move(bind));
        }
        else if (keyword == "target") {
            const auto toks = Split(rest, " \t");
            if (toks.size() < 3) {
                return std::unexpected("target needs 's<slot> <Name> <type>' -> " + line);
            }
            auto target = ParsedTarget();
            target.Slot = ParseSlot(toks[0]);
            target.Name = std::string(toks[1]);
            target.Type = std::string(toks[2]);
            result.Targets.push_back(std::move(target));
        }
        else {
            return std::unexpected("unknown @be-shader directive '" + std::string(keyword) + "' -> " + line);
        }
    }

    return result;
}

auto BeShaderTools::FindBlocks(const std::string& src) -> SourceBlocks {
    auto extract = [&](const std::string& tag, size_t from) -> std::optional<Block> {
        const auto tagPos = src.find(tag, from);
        if (tagPos == std::string::npos) {
            return std::nullopt;
        }
        const auto openPos = src.find('{', tagPos + tag.size());
        if (openPos == std::string::npos) {
            return std::nullopt;
        }
        const auto closePos = src.find('}', openPos + 1);
        if (closePos == std::string::npos) {
            return std::nullopt;
        }

        auto block = Block();
        block.Name = std::string(Trim(Take(src, tagPos + tag.size(), openPos), " \t\r\n"));
        for (const auto lineView : Split(Take(src, openPos + 1, closePos), "\n")) {
            const auto line = Trim(lineView, " \t\r");
            if (!line.empty()) {
                block.Lines.emplace_back(line);
            }
        }
        block.End = closePos + 1;
        return block;
    };

    auto result = SourceBlocks();
    result.Shader = extract("@be-shader", 0);

    auto pos = size_t(0);
    while (auto block = extract("@be-material:", pos)) {
        pos = block->End;
        result.Materials.push_back(std::move(*block));
    }

    return result;
}

auto BeShaderTools::ParseShaderFile(const std::string& src, const std::filesystem::path& path) -> std::expected<ParsedShaderFile, std::string> {
    const auto blocks = FindBlocks(src);

    auto result = ParsedShaderFile();

    if (blocks.Shader) {
        auto shader = ParseShaderBlock(*blocks.Shader);
        if (!shader) {
            return std::unexpected(shader.error());
        }
        shader->SourceFile = path;
        result.Shader = std::move(*shader);
    }

    for (const auto& block : blocks.Materials) {
        auto material = ParseMaterialBlock(block);
        if (!material) {
            return std::unexpected(material.error());
        }
        material->SourceFile = path;
        result.Materials.push_back(std::move(*material));
    }

    return result;
}

auto BeShaderTools::SchemeStructName(const std::string& schemeName) -> std::string {
    return KebabToSnake(schemeName);
}

auto BeShaderTools::BuildRootLayout(const ParsedShader& shader, const std::vector<ParsedMaterial>& materials) -> RootLayout {
    be_assert(shader.Binds.size() == materials.size(), "BuildRootLayout: bind/material count mismatch");

    auto layout = RootLayout();
    uint32_t offset = 0;

    for (size_t i = 0; i < shader.Binds.size(); ++i) {
        const auto& link = shader.Binds[i].Link;
        const auto& var  = shader.Binds[i].Var;
        const auto& material = materials[i];

        auto hasCbuffer = false;
        for (const auto& property : material.Properties) {
            if (!IsSampler(property.Type) && !IsTexture(property.Type)) {
                hasCbuffer = true;
                break;
            }
        }
        if (!hasCbuffer) {
            continue;
        }

        layout.Fields.push_back(RootField {
            .Kind      = RootFieldKind::Pointer,
            .Link      = link,
            .FieldName = KebabToPascal(link),
            .AliasName = "_" + (var.empty() ? KebabToPascal(link) : var),
            .TypeName  = SchemeStructName(material.Name),
            .Offset    = offset,
        });
        offset += 8;
    }

    for (size_t i = 0; i < shader.Binds.size(); ++i) {
        for (const auto& property : materials[i].Properties) {
            if (!IsTexture(property.Type)) {
                continue;
            }
            layout.Fields.push_back(RootField {
                .Kind         = RootFieldKind::TextureIndex,
                .Link         = shader.Binds[i].Link,
                .FieldName    = property.Name,
                .AliasName    = property.Name,
                .TypeName     = TextureSlangType(property.Type),
                .TableArray   = TextureTableArray(property.Type),
                .PropertyName = property.Name,
                .Offset       = offset,
            });
            offset += 4;
        }
    }

    for (size_t i = 0; i < shader.Binds.size(); ++i) {
        for (const auto& property : materials[i].Properties) {
            if (!IsSampler(property.Type)) {
                continue;
            }
            layout.Fields.push_back(RootField {
                .Kind         = RootFieldKind::SamplerIndex,
                .Link         = shader.Binds[i].Link,
                .FieldName    = property.Name,
                .AliasName    = property.Name,
                .TypeName     = property.Type == "comparison sampler" ? "SamplerComparisonState" : "SamplerState",
                .TableArray   = "SamplerTable",
                .PropertyName = property.Name,
                .Offset       = offset,
            });
            offset += 4;
        }
    }

    layout.Size = offset;
    return layout;
}

auto BeShaderTools::ParseFloat(const std::string& text) -> std::expected<float, std::string> {
    const auto tok = Trim(text, " \t\r\n");
    try {
        return std::stof(std::string(tok));
    } catch (const std::exception&) {
        return std::unexpected("invalid float -> " + text);
    }
}

auto BeShaderTools::ParseTuple(const std::string& text) -> std::expected<std::vector<float>, std::string> {
    const auto trimmed = Trim(text, " \t\r\n");

    if (!trimmed.empty() && trimmed.front() == '#') {
        const auto digits = trimmed.size() - 1;
        if (digits != 6 && digits != 8) {
            return std::unexpected("hex colour must have 6 or 8 digits -> " + text);
        }
        if (trimmed.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string_view::npos) {
            return std::unexpected("invalid hex digit -> " + text);
        }
        const auto hex = std::string(trimmed);
        if (digits == 6) {
            const glm::vec3 color = HexColor(hex.c_str());
            return std::vector<float>{ color.x, color.y, color.z };
        }
        const glm::vec4 color = HexColorRGBA(hex.c_str());
        return std::vector<float>{ color.x, color.y, color.z, color.w };
    }

    const auto open  = text.find('(');
    const auto close = text.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        return std::unexpected("expected a '(...)' tuple or '#RRGGBB' -> " + text);
    }

    auto result = std::vector<float>();
    for (const auto tokView : Split(Take(text, open + 1, close), ",")) {
        const auto tok = Trim(tokView, " \t\r\n");
        if (tok.empty()) {
            continue;
        }
        auto value = ParseFloat(std::string(tok));
        if (!value) {
            return std::unexpected(value.error());
        }
        result.push_back(*value);
    }
    return result;
}

auto BeShaderTools::ParseFloatArray(const std::string& text, const uint32_t componentCount) -> std::expected<std::vector<float>, std::string> {
    const auto open  = text.find('[');
    const auto close = text.rfind(']');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        return std::unexpected("expected a '[...]' array -> " + text);
    }
    const auto inner = Take(text, open + 1, close);

    auto elements = std::vector<std::string>();
    auto pos = size_t(0);
    while (pos < inner.size()) {
        const auto c = inner[pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',') {
            pos++;
            continue;
        }
        const auto start = pos;
        if (c == '(') {
            const auto groupEnd = inner.find(')', pos);
            if (groupEnd == std::string_view::npos) {
                return std::unexpected("unclosed '(' in array -> " + text);
            }
            pos = groupEnd + 1;
        }
        else {
            while (pos < inner.size() && inner[pos] != ',') {
                pos++;
            }
        }
        elements.push_back(std::string(Trim(inner.substr(start, pos - start), " \t\r\n")));
    }

    auto result = std::vector<float>();
    for (const auto& element : elements) {
        if (componentCount <= 1) {
            auto value = ParseFloat(element);
            if (!value) return std::unexpected(value.error());
            result.push_back(*value);
            continue;
        }

        auto tuple = ParseTuple(element);
        if (!tuple) return std::unexpected(tuple.error());
        if (tuple->size() != componentCount) {
            return std::unexpected("array element has wrong component count -> " + text);
        }
        result.insert(result.end(), tuple->begin(), tuple->end());
    }
    return result;
}

auto BeShaderTools::Take(const std::string_view str, const size_t start, const size_t end) -> std::string_view {
    return str.substr(start, end - start);
}

auto BeShaderTools::Trim(const std::string_view str, const char* trimmedChars) -> std::string_view {
    const auto begin = str.find_first_not_of(trimmedChars);
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = str.find_last_not_of(trimmedChars);
    return Take(str, begin, end + 1);
}

auto BeShaderTools::Split(std::string_view str, const char* delimiters) -> std::vector<std::string_view> {
    auto result = std::vector<std::string_view>();
    auto start = size_t(0);
    auto delim = str.find_first_of(delimiters, start);
    while (delim != std::string_view::npos) {
        result.push_back(Take(str, start, delim));
        start = str.find_first_not_of(delimiters, delim);
        if (start == std::string_view::npos) {
            return result;
        }
        delim = str.find_first_of(delimiters, start);
    }
    result.push_back(Take(str, start, str.size()));
    return result;
}
