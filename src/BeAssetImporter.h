#pragma once
#include <filesystem>
#include <assimp/Importer.hpp>

#include "BeModel.h"
#include "BeTexture.h"

class BeAssetImporter {
private:
    struct DecodedImage {
        uint8_t* Pixels = nullptr; // RGBA8
        uint32_t Width = 0;
        uint32_t Height = 0;
    };
    
public:
    explicit BeAssetImporter(const ComPtr<ID3D11Device>& device);
    ~BeAssetImporter() = default;
    
private:
    ComPtr<ID3D11Device> _device;
    Assimp::Importer _importer;

public:
    [[nodiscard]] auto LoadModel (const std::filesystem::path& modelPath, BeShader* usedShaderForMaterials) -> std::shared_ptr<BeModel>;
    [[nodiscard]] auto LoadTextureFromFile (const std::filesystem::path& texturePath) const -> std::shared_ptr<BeTexture>;
    
private:
    auto LoadTextureFromAssimpPath (const aiString& texPath, const aiScene* scene, const std::filesystem::path& parentPath) const -> std::shared_ptr<BeTexture>;
    auto LoadTextureFromMemoryEncoded (const uint8_t* data, uint32_t length) const -> std::shared_ptr<BeTexture>;
    auto LoadTextureFromMemoryDecoded (const uint8_t* data, uint32_t width, uint32_t height) const -> std::shared_ptr<BeTexture>;
};
