#include "BeShaderLibrary.h"

#include <sstream>
#include <fstream>
#include <ranges>

#include "BeShader.h"
#include "BeShaderTools.h"
#include "BeTexture.h"
#include "sen-rhi/SenBackend.h"
#include "sen-rhi/SenShaderCompiler.h"

std::unordered_map<std::filesystem::path, std::string>          BeShaderLibrary::_shaderSources;
std::unordered_map<std::string, std::unique_ptr<BeShader>>      BeShaderLibrary::_shaders;
std::unordered_map<std::string, BeMaterialScheme>              BeShaderLibrary::_materialSchemes;
std::unordered_map<std::string, std::shared_ptr<BeTexture>>    BeShaderLibrary::_defaultTextures;
std::unordered_map<std::string, SenSampler>                    BeShaderLibrary::_samplers;

auto BeShaderLibrary::LoadShaderFiles(const std::vector<std::filesystem::path>& filePaths) -> void {

    // collect sources
    auto sourcesToIndex = std::vector<std::pair<std::filesystem::path, std::string>>();
    for (const auto& path : filePaths) {
        if (_shaderSources.contains(path))
            continue;

        be_assert(std::filesystem::exists(path), path);

        auto file = std::ifstream(path);
        auto buffer = std::stringstream();
        buffer << file.rdbuf();
        auto src = buffer.str();

        _shaderSources[path] = src;
        sourcesToIndex.emplace_back(path, src);
    }

    // index material schemes
    for (const auto& src : sourcesToIndex | std::views::values) {
        auto materials = BeShaderTools::ParseMaterials(src);
        be_assert(materials.has_value(), materials.error());

        for (const auto& material : materials.value()) {
            _materialSchemes[material.Name] = BeMaterialScheme::Create(material.Name, material.Properties);
        }
    }

    // index shaders
    for (const auto& [path, src] : sourcesToIndex) {
        if (src.find("@be-shader") == std::string::npos)
            continue;

        auto shader = BeShader::Create(path);
        auto name = shader->Name;
        _shaders[std::move(name)] = std::move(shader);
    }
}

auto BeShaderLibrary::LoadShaderDirectory(const std::filesystem::path& dir) -> void {
    be_assert(std::filesystem::exists(dir), dir);

    SenShaderCompiler::AddSearchPath(dir);

    auto filePaths = std::vector<std::filesystem::path>();
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".hlsl") {
            filePaths.push_back(entry.path());
        }
    }

    LoadShaderFiles(filePaths);
}

auto BeShaderLibrary::GetShader(std::string_view name) -> raw_ptr<BeShader> {
    be_assert(_shaders.contains(std::string(name)), name);
    return _shaders.at(std::string(name)).get();
}

auto BeShaderLibrary::GetMaterialScheme(std::string_view name) -> const BeMaterialScheme& {
    be_assert(_materialSchemes.contains(std::string(name)), name);
    return _materialSchemes.at(std::string(name));
}

auto BeShaderLibrary::RegisterBuiltinDefaultTextures() -> void {

    struct BuiltinDefaultTexture {
        std::string_view Name;
        glm::vec4 Color;
        SenTextureUsage Usage = SenTextureUsage::ShaderResource;
        bool Cubemap = false;
    };

    const BuiltinDefaultTexture builtins[] = {
        { "white", glm::vec4(1.f) },
        { "black", glm::vec4(0.f, 0.f, 0.f, 1.f) },
        { "storage-black", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource | SenTextureUsage::Storage },
        { "black-cube", glm::vec4(0.f, 0.f, 0.f, 1.f), SenTextureUsage::ShaderResource, true },
        { "default-orm", glm::vec4(0.f, 1.f, 1.f, 1.f) },
        { "flat-normal", glm::vec4(0.5f, 0.5f, 1.f, 1.f) },
    };

    for (const auto& builtin : builtins) {
        RegisterDefaultTexture(
            builtin.Name,
            BeTexture::Create(std::string(builtin.Name))
            .SetSize(1, 1)
            .SetCubemap(builtin.Cubemap)
            .SetUsage(builtin.Usage)
            .SetFormat(SenFormat::RGBA8_Unorm)
            .FillWithColor(builtin.Color)
            .Build()
        );
    }
}

auto BeShaderLibrary::RegisterDefaultTexture(std::string_view name, std::shared_ptr<BeTexture> texture) -> void {
    _defaultTextures[std::string(name)] = std::move(texture);
}

auto BeShaderLibrary::GetSampler(std::string_view samplerDescString) -> SenSampler {

    const auto key = std::string(samplerDescString);

    if (_samplers.contains(key)) {
        return _samplers[key];
    }

    auto tokens = BeShaderTools::Split(samplerDescString, "-");
    be_assert(
        tokens.size() == 2 || tokens.size() == 3,
        "Invalid samplerDescString. Expected format: filter-address[-cmp]",
        samplerDescString,
        tokens.size()
    );

    auto filterToken   = std::string(tokens[0]);
    auto addressToken  = std::string(tokens[1]);
    auto hasComparison = tokens.size() == 3 && tokens[2] == "cmp";

    SenFilter filter = SenFilter::Linear;
    if (filterToken == "point") {
        filter = SenFilter::Point;
    } else if (filterToken == "linear") {
        filter = SenFilter::Linear;
    } else if (filterToken == "anisotropic") {
        filter = SenFilter::Anisotropic;
    } else {
        be_assert(false, "Unknown filter token", filterToken);
    }

    SenAddressMode address = SenAddressMode::Clamp;
    if (addressToken == "wrap") {
        address = SenAddressMode::Wrap;
    } else if (addressToken == "clamp") {
        address = SenAddressMode::Clamp;
    } else if (addressToken == "mirror") {
        address = SenAddressMode::Mirror;
    } else {
        be_assert(false, "Unknown address token", addressToken);
    }

    auto sampler = SenBackend::CreateSampler({
        .Filter     = filter,
        .Address    = address,
        .Comparison = hasComparison,
    });

    _samplers[key] = sampler;
    return sampler;
}

auto BeShaderLibrary::Shutdown() -> void {
    _shaders.clear();
    _materialSchemes.clear();
    _shaderSources.clear();
    _defaultTextures.clear();
    _samplers.clear();
}
