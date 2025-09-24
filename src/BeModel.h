#pragma once
#include <memory>
#include <filesystem>
#include <assimp/scene.h>

#include "BeShader.h"


struct aiMaterial;

class BeModel {
    struct DecodedImage {
        uint8_t* Pixels = nullptr; // RGBA8
        uint32_t Width = 0;
        uint32_t Height = 0;
    };
    
public:
    struct BeMeshInstruction {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
        ComPtr<ID3D11ShaderResourceView> DiffuseTexture = nullptr; // temporary
    };
    
    BeModel(const std::filesystem::path& modelPath, ID3D11Device* device);
    ~BeModel() = default;

    std::vector<BeMeshInstruction> MeshInstructions;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
    
    uint32_t Stride;
    uint32_t Offset;

private:
    auto ToRGBA8 (const aiTexture* texture, DecodedImage& out) -> bool;
    auto FlipImageVertically(DecodedImage& image) -> void;
    //auto GetTexture (const aiScene* scene, const aiMaterial* material) -> void;
};
