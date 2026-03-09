#include "BeAssetRegistry.h"

#include <fstream>

#include "BeShader.h"
#include "BeShaderTools.h"
#include "BeRenderer.h"
#include "sen-rhi/SenBackend.h"

std::weak_ptr<BeRenderer> BeAssetRegistry::_renderer;

std::unordered_map<std::filesystem::path, std::string> BeAssetRegistry::_shaderSources;

std::unordered_map<std::string, BeMaterialScheme> BeAssetRegistry::_materialSchemes;
std::unordered_map<std::string, SenSampler> BeAssetRegistry::_samplers;
std::unordered_map<std::string, std::shared_ptr<BeShader>> BeAssetRegistry::_shaders;
std::unordered_map<std::string, std::shared_ptr<BeMaterial>> BeAssetRegistry::_materials;
std::unordered_map<std::string, std::shared_ptr<BeTexture>> BeAssetRegistry::_textures;
std::unordered_map<std::string, std::shared_ptr<BeProp>> BeAssetRegistry::_props;

auto BeAssetRegistry::Shutdown() -> void {
    _props.clear();
    _materials.clear();
    _textures.clear();
    _samplers.clear();
    _shaders.clear();
    _materialSchemes.clear();
    _shaderSources.clear();
    _renderer.reset();
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
        
        auto startPos = src.find("@be-material:");
        while (startPos != std::string::npos) {
            auto endPos = src.find("@be-end", startPos);
            assert(endPos != std::string::npos);
            
            auto nameStart = src.find(" ", startPos);
            assert(nameStart != std::string::npos);
            nameStart++;
            auto jsonStart = src.find('\n', startPos);
            assert(jsonStart != std::string::npos && jsonStart < endPos);
            
            auto materialNameRaw = src.substr(nameStart, jsonStart - nameStart);
            auto materialName = std::string(BeShaderTools::Trim(materialNameRaw, " \t"));
            
            jsonStart++; // Move past newline
            auto jsonContent = src.substr(jsonStart, endPos - jsonStart);
    
            jsonContent.erase(0, jsonContent.find_first_not_of(" \t\r\n"));
            jsonContent.erase(jsonContent.find_last_not_of(" \t\r\n") + 1);
            
            auto json = Json();
            
            try {
                json = Json::parse(jsonContent, nullptr, true, true, true);
            } catch (const Json::parse_error& e) {
                const auto msg = e.what();
                assert(false);
            }
            
            auto materialScheme = BeMaterialScheme::CreateFromJson(materialName, json);
            _materialSchemes[materialName] = materialScheme;
            
            startPos = src.find("@be-material:", endPos);
        }
        
    } 
    
    // index shaders
    for (const auto& [path, src] : sourcesToIndex) {
        if (src.find("@be-shader:") == std::string::npos) 
            continue;
        
        auto shader = BeShader::Create(path, *_renderer.lock());
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

    auto renderer = _renderer.lock();
    be_assert(renderer, "Renderer couldn't be locked");

    auto sampler = SenBackend::CreateSampler({
        .Filter     = filter,
        .Address    = address,
        .Comparison = hasComparison,
    });

    _samplers[key] = sampler;
    return sampler;
}
