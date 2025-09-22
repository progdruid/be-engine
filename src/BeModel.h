#pragma once
#include <memory>
#include <filesystem>
#include "BeShader.h"

class BeModel {
public:
    struct BeMeshInstruction {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
    };
    
    BeModel(const std::shared_ptr<BeShader>& shader, const std::filesystem::path& modelPath, ID3D11Device* device);
    ~BeModel() = default;

    std::shared_ptr<BeShader> Shader;
    std::vector<BeMeshInstruction> DrawInstructions;
    std::vector<BeFullVertex> FullVertices;
    std::vector<uint32_t> Indices;
    
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    uint32_t Stride;
    uint32_t Offset;
};
