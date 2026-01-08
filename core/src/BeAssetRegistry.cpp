#include "BeAssetRegistry.h"

#include <fstream>

#include "BeShader.h"
#include "BeShaderTools.h"
#include "BeRenderer.h"

std::weak_ptr<BeRenderer> BeAssetRegistry::_renderer;

std::unordered_map<std::filesystem::path, std::string> BeAssetRegistry::_shaderSources;

std::unordered_map<std::string, std::shared_ptr<BeShader>> BeAssetRegistry::_shaders;
std::unordered_map<std::string, BeMaterialScheme> BeAssetRegistry::_materialSchemes;
std::unordered_map<std::string, ComPtr<ID3D11SamplerState>> BeAssetRegistry::_samplers;
std::unordered_map<std::string, std::shared_ptr<BeMaterial>> BeAssetRegistry::_materials;
std::unordered_map<std::string, std::shared_ptr<BeTexture>> BeAssetRegistry::_textures;
std::unordered_map<std::string, std::shared_ptr<BeModel>> BeAssetRegistry::_models;

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

auto BeAssetRegistry::GetSampler(std::string_view samplerDescString) -> ComPtr<ID3D11SamplerState> {

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

    auto filterToken = std::string(tokens[0]);
    auto addressToken = std::string(tokens[1]);
    auto hasComparison = tokens.size() == 3 && tokens[2] == "cmp";

    
    D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    if (filterToken == "point") {
        filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    } else if (filterToken == "linear") {
        filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    // todo: add anisotropic
    } else {
        be_assert(false, "Unknown filter token", filterToken);
    }
    
    
    D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
    if (addressToken == "wrap") {
        addressMode = D3D11_TEXTURE_ADDRESS_WRAP;
    } else if (addressToken == "clamp") {
        addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
    } else if (addressToken == "mirror") {
        addressMode = D3D11_TEXTURE_ADDRESS_MIRROR;
    } else {
        be_assert(false, "Unknown address token", addressToken);
    }
    
    
    if (hasComparison) {
        if (filter == D3D11_FILTER_MIN_MAG_MIP_POINT) {
            filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        } else if (filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR) {
            filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        } else if (filter == D3D11_FILTER_ANISOTROPIC) {
            filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
        } else {
            be_assert(false, "Unreachable, wrong filter", filter);
        }
    }
    
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = filter;
    samplerDesc.AddressU = addressMode;
    samplerDesc.AddressV = addressMode;
    samplerDesc.AddressW = addressMode;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = hasComparison ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    
    auto renderer = _renderer.lock();
    be_assert(renderer, "Renderer couldn't be locked");

    auto device = renderer->GetDevice();
    ComPtr<ID3D11SamplerState> samplerState;

    auto hr = device->CreateSamplerState(&samplerDesc, &samplerState);
    be_assert(SUCCEEDED(hr), "Failed to create sampler state", samplerDescString);

    _samplers[key] = samplerState;
    return samplerState;
}
