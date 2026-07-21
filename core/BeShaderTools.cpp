#include "BeShaderTools.h"

#include <fstream>
#include <sstream>

#include "umbrellas/include-libassert.h"

namespace {
    auto ParseSlot(std::string_view tok) -> uint8_t {
        if (!tok.empty() && (tok.front() == 's' || tok.front() == 'S')) {
            tok = tok.substr(1);
        }
        return static_cast<uint8_t>(std::stoi(std::string(tok)));
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
        try {
            result.ArrayLength = std::stoul(std::string(Trim(Take(result.Type, lb + 1, rb), " \t")));
        } catch (const std::exception&) {
            return std::unexpected("invalid array length -> " + text);
        }
        if (result.ArrayLength == 0) {
            return std::unexpected("array length must be > 0 -> " + text);
        }
        result.Type = std::string(Trim(std::string_view(result.Type).substr(0, lb), " \t"));
    }

    return result;
}

auto BeShaderTools::ParseMaterials(const std::string& src) -> std::expected<std::vector<ParsedMaterial>, std::string> {
    auto result = std::vector<ParsedMaterial>();

    auto pos = size_t(0);
    while (const auto block = FindBlock(src, "@be-material:", pos)) {
        auto mat = ParsedMaterial();
        mat.Name = block->Name;
        for (const auto& line : block->Lines) {
            auto property = ParseMaterialProperty(line);
            if (!property) {
                return std::unexpected("material '" + mat.Name + "' -> " + property.error());
            }
            mat.Properties.push_back(std::move(*property));
        }

        result.push_back(std::move(mat));
        pos = block->End;
    }

    return result;
}

auto BeShaderTools::ParseShader(const std::string& src) -> std::expected<ParsedShader, std::string> {
    auto result = ParsedShader();

    auto block = FindBlock(src, "@be-shader", 0);
    if (!block) {
        return std::unexpected(std::string("no @be-shader block found"));
    }
    result.Name = block->Name;

    for (const auto& line : block->Lines) {
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

auto BeShaderTools::FindBlock(const std::string& src, const std::string& tag, size_t from) -> std::optional<Block> {
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
}

auto BeShaderTools::ParseFloatList(const std::string& text) -> std::expected<std::vector<float>, std::string> {
    const auto open  = text.find('(');
    const auto close = text.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        return std::unexpected("expected a '(...)' float list -> " + text);
    }

    auto result = std::vector<float>();
    for (const auto tokView : Split(Take(text, open + 1, close), ",")) {
        const auto tok = Trim(tokView, " \t\r\n");
        if (tok.empty()) {
            continue;
        }
        try {
            result.push_back(std::stof(std::string(tok)));
        } catch (const std::exception&) {
            return std::unexpected("invalid float '" + std::string(tok) + "' -> " + text);
        }
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

    auto result = std::vector<float>();

    if (componentCount <= 1) {
        for (const auto tokView : Split(inner, ",")) {
            const auto tok = Trim(tokView, " \t\r\n");
            if (tok.empty()) {
                continue;
            }
            try {
                result.push_back(std::stof(std::string(tok)));
            } catch (const std::exception&) {
                return std::unexpected("invalid float '" + std::string(tok) + "' -> " + text);
            }
        }
        return result;
    }

    auto pos = size_t(0);
    while (true) {
        const auto groupStart = inner.find('(', pos);
        if (groupStart == std::string_view::npos) {
            break;
        }
        const auto groupEnd = inner.find(')', groupStart);
        if (groupEnd == std::string_view::npos) {
            return std::unexpected("unclosed '(' in array -> " + text);
        }
        auto group = ParseFloatList(std::string(inner.substr(groupStart, groupEnd - groupStart + 1)));
        if (!group) {
            return std::unexpected(group.error());
        }
        if (group->size() != componentCount) {
            return std::unexpected("array element has wrong component count -> " + text);
        }
        result.insert(result.end(), group->begin(), group->end());
        pos = groupEnd + 1;
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
