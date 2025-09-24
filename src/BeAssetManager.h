#pragma once
#include <filesystem>
#include <assimp/Importer.hpp>

#include "BeModel.h"
#include "BeTexture.h"

class BeAssetManager {
    struct DecodedImage {
        uint8_t* Pixels = nullptr; // RGBA8
        uint32_t Width = 0;
        uint32_t Height = 0;
    };
    
public:
    static std::unique_ptr<BeAssetManager> Ins;
    
public:
    explicit BeAssetManager(const ComPtr<ID3D11Device>& device);
    ~BeAssetManager() = default;
    
private:
    ComPtr<ID3D11Device> _device;
    Assimp::Importer _importer;

    std::unordered_map<std::string, BeModel> _models;
    std::unordered_map<std::string, BeTexture> _textures;
    
public:
    auto LoadModel (const std::string& name, const std::filesystem::path& modelPath) -> void;
    auto GetModel (const std::string& name) -> BeModel&;

    auto LoadTextureFromFile (const std::string& name, const std::filesystem::path& texturePath) -> BeTexture&;
    auto GetTexture (const std::string& name) -> BeTexture&;
    
private:
    auto LoadTextureFromAssimpPath (const std::string& name, const aiString& texPath, const aiScene* scene, const std::filesystem::path& parentPath) -> BeTexture&;
    auto LoadTextureFromMemoryEncoded (const std::string& name, const uint8_t* data, uint32_t length) -> BeTexture&;
    auto LoadTextureFromMemoryDecoded (const std::string& name, const uint8_t* data, uint32_t width, uint32_t height) ->BeTexture&;

    auto FlipTextureVertically(BeTexture& texture) -> void;
    auto CreateSRV (BeTexture& texture) -> void;
};
