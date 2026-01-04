#include "BeAssetRegistry.h"

#include <fstream>

#include "BeShader.h"
#include "BeShaderTools.h"

std::unordered_map<std::filesystem::path, std::string> BeAssetRegistry::_shaderSources;
std::unordered_map<std::string, std::shared_ptr<BeShader>> BeAssetRegistry::_shaders;
std::unordered_map<std::string, BeMaterialScheme> BeAssetRegistry::_materialSchemes;
std::unordered_map<std::string, std::shared_ptr<BeMaterial>> BeAssetRegistry::_materials;
std::unordered_map<std::string, std::shared_ptr<BeTexture>> BeAssetRegistry::_textures;
std::unordered_map<std::string, std::shared_ptr<BeModel>> BeAssetRegistry::_models;

auto BeAssetRegistry::IndexShaderFiles(const std::vector<std::filesystem::path>& filePaths, const BeRenderer& renderer) -> void {
    
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
        
        auto shader = BeShader::Create(path, renderer);
        
        auto shaderName_Temporary = path.stem().string();
        _shaders[shaderName_Temporary] = shader;
    }
}
