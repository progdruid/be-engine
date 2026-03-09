#pragma once
#include <filesystem>
#include <memory>
#include <vector>

#include "BeMesh.h"
#include "BeMaterialBinding.h"

struct aiScene;
struct aiString;
class BeTexture;
class BeShader;
class BeMaterial;
class BeRenderer;

struct BePropSlice {
    std::shared_ptr<BeMaterial> Material;
    BeMaterialBinding Binding;
    bool TwoSided = false;
};

struct BeProp {
    std::shared_ptr<BeMesh>                  Mesh;
    std::vector<BePropSlice>                 Slices;
    std::shared_ptr<BeShader>                Shader;
    std::vector<std::shared_ptr<BeMaterial>> Materials;

    static auto Create(
        const std::filesystem::path& modelPath,
        std::weak_ptr<BeShader> usedShaderForMaterials,
        const BeRenderer& renderer
    ) -> std::shared_ptr<BeProp>;

    static auto FromMesh(
        std::shared_ptr<BeMesh> mesh,
        std::weak_ptr<BeShader> shader,
        const BeRenderer& renderer
    ) -> std::shared_ptr<BeProp>;

    static auto LoadTextureFromAssimpPath(
        const aiString& texPath,
        const aiScene* scene,
        const std::filesystem::path& parentPath,
        const BeRenderer& renderer
    ) -> std::shared_ptr<BeTexture>;

    BeProp() = default;
    ~BeProp() = default;
};
