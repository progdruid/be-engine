#pragma once
#include <filesystem>
#include <functional>
#include <common.hpp>
#include <assimp/Importer.hpp>

struct aiMaterial;
struct aiScene;
struct BeProp;
struct BePropSurface;
class BeTexture;
class BeMaterial;
class BeRenderer;
struct BeShader;

class BeAssimpImporter {
    expose
    using MaterialExtractFunction = std::function<std::shared_ptr<BeMaterial>(
        aiMaterial const*,
        aiScene const*,
        const std::filesystem::path&,
        const BePropSurface&
    )>;

    hide
    Assimp::Importer _importer;

    expose
    explicit BeAssimpImporter();
    ~BeAssimpImporter();

    expose
    auto LoadProp(const std::filesystem::path& modelPath, const MaterialExtractFunction& materialExtractFunction) -> std::shared_ptr<BeProp>;
    
    expose
    static auto LoadTextureFromAssimpPath(
        const aiString& texPath,
        const aiScene* scene,
        const std::filesystem::path& parentPath
    ) -> std::shared_ptr<BeTexture>;
};
