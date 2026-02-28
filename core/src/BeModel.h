#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>
#include <umbrellas/include-glm.h>

struct aiScene;
struct aiString;
class BeTexture;
class BeShader;
class BeMaterial;
class BeRenderer;

struct BeFullVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color     {1, 1, 1, 1};
    glm::vec2 UV0       {0, 0};
    glm::vec2 UV1       {0, 0};
    glm::vec2 UV2       {0, 0};
};

struct BeDrawSlice {
    uint32_t IndexCount;
    uint32_t StartIndexLocation;
    int32_t BaseVertexLocation;
    std::shared_ptr<BeMaterial> Material = nullptr;
    bool TwoSided = false;
};

struct BeModel {

    static auto Create(
        const std::filesystem::path& modelPath,
        std::weak_ptr<BeShader> usedShaderForMaterials,
        BeRenderer& renderer
    ) -> std::shared_ptr<BeModel>;

    static auto LoadTextureFromAssimpPath(
        const aiString& texPath,
        const aiScene* scene,
        const std::filesystem::path& parentPath,
        BeRenderer& renderer
    ) -> std::shared_ptr<BeTexture>;

    std::vector<BeDrawSlice> DrawSlices;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
    std::vector<std::shared_ptr<BeMaterial>> Materials;
    std::shared_ptr<BeShader> Shader;

    BeModel() = default;
    ~BeModel() = default;
};
