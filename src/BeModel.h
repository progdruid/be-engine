#pragma once
#include <memory>
#include <filesystem>
#include <assimp/scene.h>

#include "BeShader.h"
#include "BeTexture.h"


struct BeModel {
    struct BeMeshInstruction {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
        BeTexture* DiffuseTexture = nullptr; // temporary
    };

    BeModel() = default;
    ~BeModel() = default;

    std::vector<BeMeshInstruction> MeshInstructions;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
    
    uint32_t Stride;
    uint32_t Offset;
};
