#pragma once
#include <filesystem>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include <umbrellas/include-glm.h>
#include <assimp/scene.h>

#include "BeMaterial.h"

using Microsoft::WRL::ComPtr;

class BeAssetRegistry;

struct BeFullVertex {
    glm::vec3 Position;                 // 0
    glm::vec3 Normal;                   // 12
    glm::vec4 Color     {1, 1, 1, 1};   // 24
    glm::vec2 UV0       {0, 0};         // 40
    glm::vec2 UV1       {0, 0};         // 48
    glm::vec2 UV2       {0, 0};         // 56
};

struct BeModel {
    struct BeDrawSlice {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
        std::shared_ptr<BeMaterial> Material = nullptr;
    };

    // Static
    static auto Create(
        const std::filesystem::path& modelPath,
        std::weak_ptr<BeShader> usedShaderForMaterials,
        std::weak_ptr<BeAssetRegistry> registry,
        ComPtr<ID3D11Device> device
    ) -> std::shared_ptr<BeModel>;
    static auto LoadTextureFromAssimpPath(
        const struct aiString& texPath,
        const struct aiScene* scene,
        const std::filesystem::path& parentPath,
        std::weak_ptr<BeAssetRegistry> registry,
        ComPtr<ID3D11Device> device
    ) -> std::shared_ptr<BeRenderResource>;

    std::vector<BeDrawSlice> DrawSlices;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
    std::vector<std::shared_ptr<BeMaterial>> Materials;
    std::shared_ptr<BeShader> Shader;

    BeModel() = default;
    ~BeModel() = default;
};
