#pragma once
#include <filesystem>
#include <functional>
#include <umbrellas/common.hpp>
#include <assimp/Importer.hpp>

struct aiMaterial;
struct aiScene;
struct BeProp;
class BeTexture;
class BeMaterial;
class BeRenderer;
class BeShader;

class BeAssimpImporter {
    hide
    Assimp::Importer _importer;
    
    expose
    explicit BeAssimpImporter();
    ~BeAssimpImporter();
    
    expose
    auto LoadProp (
        const std::filesystem::path& modelPath,
        std::weak_ptr<BeShader> usedShaderForMaterials,
        const std::function<std::shared_ptr<BeMaterial>(aiMaterial const*, aiScene const*, const std::filesystem::path&)>&
        materialExtractFunction
    ) -> std::shared_ptr<BeProp>;
    
    expose
    static auto LoadTextureFromAssimpPath(
        const aiString& texPath,
        const aiScene* scene,
        const std::filesystem::path& parentPath
    ) -> std::shared_ptr<BeTexture>;
};
