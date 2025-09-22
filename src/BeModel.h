#pragma once
#include <memory>
#include <filesystem>
#include "BeShader.h"

class BeModel {
    struct DrawInstruction {
        uint32_t IndexCount;
        uint32_t StartIndexLocation;
        int32_t BaseVertexLocation;
    };
    
public:
    BeModel(const std::shared_ptr<BeShader>& shader, const std::filesystem::path& modelPath, ID3D11Device* device);
    ~BeModel() = default;

    std::shared_ptr<BeShader> Shader;
    std::vector<DrawInstruction> DrawInstructions;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    uint32_t Stride;
    uint32_t Offset;
};
