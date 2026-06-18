#include "BeAssetRegistry.h"

#include <sstream>
#include <fstream>
#include <ranges>

#include "BeShader.h"
#include "BeShaderTools.h"
#include "sen-rhi/SenBackend.h"

std::unordered_map<std::string, std::shared_ptr<BeTexture>> BeAssetRegistry::_defaultTextures;
std::unordered_map<std::string, SenSampler>                 BeAssetRegistry::_samplers;

BeAssetRegistry::BeAssetRegistry()  = default;
BeAssetRegistry::~BeAssetRegistry() {
    _props.clear();
    _materials.clear();
    _textures.clear();
    _shaders.clear();
    _materialSchemes.clear();
    _shaderSources.clear();
}

auto BeAssetRegistry::RegisterDefaultTexture(std::string_view name, std::shared_ptr<BeTexture> texture) -> void {
    _defaultTextures[std::string(name)] = std::move(texture);
}

auto BeAssetRegistry::StaticShutdown() -> void {
    _defaultTextures.clear();
    _samplers.clear();
}

auto BeAssetRegistry::IndexShaderFiles(const std::vector<std::filesystem::path>& filePaths) -> void {

    // collect sources
    auto sourcesToIndex = std::vector<std::pair<std::filesystem::path, std::string>>();
    for (const auto& path : filePaths) {
        if (_shaderSources.contains(path))
            continue;

        assert(std::filesystem::exists(path));

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

        auto shader = BeShader::Create(path, *this);
        _shaders[shader->Name] = shader;
    }
}

auto BeAssetRegistry::GetSampler(std::string_view samplerDescString) -> SenSampler {

    auto key = std::string(samplerDescString);

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
