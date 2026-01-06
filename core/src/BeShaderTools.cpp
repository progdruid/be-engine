#include "BeShaderTools.h"

#include <fstream>

#include "umbrellas/include-libassert.h"

auto BeShaderTools::ReadFile(const std::filesystem::path& path) -> std::string {
    be_assert(std::filesystem::exists(path), "file does not exist", path);
    
    auto file = std::ifstream(path);
    auto buffer = std::stringstream();
    buffer << file.rdbuf();
    auto src = buffer.str();
    return src;
}

auto BeShaderTools::ParseMaterialProperty(const std::string& text) -> ParsedMaterialProperty {
    auto result = ParsedMaterialProperty();
    
    const auto parts = Split(text, ":=");
    assert(!parts.empty());
    
    result.Name = Trim(parts[0], " \t\n\r");
    
    const auto typeParts = Split(Trim(parts[1], " \n\r\t"), "()");
    assert(!typeParts.empty());
    
    result.Type = Trim(typeParts[0], " \t\n\r");
    
    if (typeParts.size() > 1) {
        result.Slot = std::stoi(std::string(typeParts[1]));
    }
    if (parts.size() > 2) {
        result.Default = Trim(parts[2], " \n\r\t");
    }
    
    return result;
}

auto BeShaderTools::ParseFor(const std::string& src, const std::string& target) -> std::pair<Json, std::string> {
    const auto& startTag = target;
    const auto& endTag = std::string("@be-end");
    
    auto metadata = Json::object();
    
    const auto startPos = src.find(startTag);
    if (startPos == std::string::npos) return metadata;
    const auto endPos = src.find(endTag, startPos);
    if (endPos == std::string::npos) return metadata;
    
    auto jsonStart = src.find('\n', startPos);
    be_assert(jsonStart != std::string::npos && jsonStart < endPos);
    
    auto nameStart = startPos + target.length();
    auto nameToken = std::string(BeShaderTools::Trim(src.substr(nameStart, jsonStart - nameStart), " \t\n\r"));
    
    jsonStart++; // Move past newline
    
    auto jsonContent = src.substr(jsonStart, endPos - jsonStart);
    
    jsonContent.erase(0, jsonContent.find_first_not_of(" \t\r\n"));
    jsonContent.erase(jsonContent.find_last_not_of(" \t\r\n") + 1);
    
    try {
        metadata = Json::parse(jsonContent, nullptr, true, true, true);
    } catch (const Json::parse_error& e) {
        const auto msg = e.what();
        throw std::runtime_error("Failed to parse shader header JSON: " + std::string(msg));
    }
    
    return {metadata, nameToken};
}

auto BeShaderTools::Take(const std::string_view str, const size_t start, const size_t end) -> std::string_view {
    return str.substr(start, end - start);
}

auto BeShaderTools::Trim(const std::string_view str, const char* trimmedChars) -> std::string_view {
    return Take(str, str.find_first_not_of(trimmedChars), str.find_last_not_of(trimmedChars) + 1);
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